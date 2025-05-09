
#include <vxWorks.h>
#include <stdio.h>
#include <sockLib.h>
#include <inetLib.h>
#include <taskLib.h>
#include <socket.h>
#include <stdlib.h>
#include <sysLib.h>
#include <logLib.h>
#include <errno.h>
#include <string.h>
#include "wdLib.h"
#include "in.h"
#include "ioLib.h"
#include <unistd.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <selectLib.h>
#include <tickLib.h>
#include <msgQLib.h>
#include "pack2.h"


#define BACKLOG   16


typedef enum {
    STATE_INIT = 0,
    STATE_TCP_OPEN,
    STATE_TCP_CONN,
    STATE_TCP_CLOSE,
    STATE_TCP_WAIT,
    STATE_RW_DATA,
    STATE_MAX
} sock_state_enum;

const char* STATE_NAMES[] = {
    "STATE_INIT",        // 0
    "STATE_TCP_OPEN",    // 1
    "STATE_TCP_CONN",    // 2
    "STATE_TCP_CLOSE",   // 3
    "STATE_TCP_WAIT",    // 4
    "STATE_RW_DATA",     // 5
};

const int portdata_array[] = {950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960, 961, 962, 963, 964, 965};
const int portcmd_array[] = {966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981};



#define TCP_SERVER_TASK_PRIO        (95)
#define UART_TX_TASK_PRIO           (85)
#define UART_RX_TASK_PRIO           (55)
#define UART_RX_FORWARD_TASK_PRIO   (75)

/*  */
#define UART_TX_TASK_MIN_DELAY              (5)
#define UART_RX_TASK_MIN_DELAY              (5)
#define UART_RX_FORWARD_TASK_MIN_DELAY      (50)

#define TCP_SERVER_TASK_STACK 4096
#define UART_TASK_STACK 8192

#define UART_FPGA_BUFFER            (1024)
static char uart_fpga_rxbuf[UART_FPGA_BUFFER];
static char uart_fpga_txbuf[UART_FPGA_BUFFER];

#define NET_BUFFER            (1024)
static char net_tx_buf[NET_BUFFER];

#define SOCK_DATA_BUFFER        (2048)
static char sock_data_tmp[SOCK_DATA_BUFFER];

#define SOCK_cmd_BUFFER        (256)
static char cmd_tmp[SOCK_cmd_BUFFER];

UART_Config_Params uart_instances[NUM_PORTS];

int msToTicks(int ms)
{
    return (ms * sysClkRateGet() + 999) / 1000;
}

// 入队封装
void uart_ring_buffer_enqueue(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size)
{
    if (buffer == NULL || data == NULL || size == 0)
        return;
    ring_buffer_queue_arr(buffer, data, size);
}

// 出队封装
ring_buffer_size_t uart_ring_buffer_dequeue(ring_buffer_t *buffer, char *data, ring_buffer_size_t len)
{
    if (buffer == NULL || data == NULL || len == 0)
        return 0;
    return ring_buffer_dequeue_arr(buffer, data, len);
}



void uart_info_send(uint8_t i)
{
	unsigned char send_buf[4] = {0x26, 0x00, 0x00, 0x81};	
	int send_len = sizeof(send_buf);
	if (send(uart_instances[i].cmd_client_fd, send_buf, send_len, 0) != send_len) {
		perror("send failed");
	}  
}

int create_tcp_server_socket(int *sock_fd)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket creation failed");
        return -1;
    }
    *sock_fd = sock;
    return 1;
}


int bind_tcp_server_socket(int sock_fd, int port)
{
    struct sockaddr_in server_addr;
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    printf("bind sock: %d  port %d \n",sock_fd ,port);
    int ret = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0) {
        perror("bind failed");
        return -1;
    }
    return 0;
}

int listen_tcp_server_socket(int sock_fd)
{
    printf("listen sock: %d \n",sock_fd);
    int ret = listen(sock_fd, BACKLOG);
    if (ret < 0) {
        perror("listen failed");
        return -1;
    }
    return 0;
}

// 状态变更helper
void set_state(uint8_t *var, sock_state_enum new_state, int idx, const char *side, uint16_t port)
{
    if (*var != new_state) {
        // printf("[%s %d port %u] State: %s -> %s\n",
        //     side, idx, port, STATE_NAMES[*var], STATE_NAMES[new_state]);
        *var = new_state;
    }
}
// --------------------- sock_cmd 任务 ------------------------
void multi_tcp_cmd_servers_loop(int unused)
{
    int i;
    for (i = 0; i < NUM_PORTS; ++i) {
        uart_instances[i].cmd_client_fd = -1;
        uart_instances[i].sock_cmd_state = STATE_TCP_OPEN;
    }
    taskDelay(30);
    printf("tcp cmd loop start ... \n");
    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        int max_fd = 0;
        for (i = 0; i < NUM_PORTS; ++i) {
            if (uart_instances[i].sock_cmd > 0 && uart_instances[i].cmd_client_fd < 0) {
                FD_SET(uart_instances[i].sock_cmd, &rfds);
                if (uart_instances[i].sock_cmd > max_fd)
                    max_fd = uart_instances[i].sock_cmd;
            }
            if (uart_instances[i].cmd_client_fd >= 0) {
                FD_SET(uart_instances[i].cmd_client_fd, &rfds);
                if (uart_instances[i].cmd_client_fd > max_fd)
                    max_fd = uart_instances[i].cmd_client_fd;
            }
        }
        int ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select error (cmd)");
            taskDelay(5);
            continue;
        }
        for (i = 0; i < NUM_PORTS; ++i) {
            UART_Config_Params *uart = &uart_instances[i];
            if (uart->sock_cmd > 0 && uart->cmd_client_fd < 0 && FD_ISSET(uart->sock_cmd, &rfds)) {
                struct sockaddr_in caddr;
                int clen = sizeof(caddr);
                int client_fd = accept(uart->sock_cmd, (struct sockaddr*)&caddr, &clen);
                if (client_fd >= 0) {
                    uart->cmd_client_fd = client_fd;
                    uart_info_send(i);
                    set_state(&uart->sock_cmd_state, STATE_TCP_CONN, i, "CMD", uart->sock_cmd_port);
                    printf("uart[%d] sock_cmd (cmd_port %u) client connected, fd=%d\n",
                            i, uart->sock_cmd_port, client_fd);
                }
            }
            // 数据与断开
            if (uart->cmd_client_fd >= 0 && FD_ISSET(uart->cmd_client_fd, &rfds)) {
                int n = recv(uart->cmd_client_fd, cmd_tmp, sizeof(cmd_tmp), MSG_PEEK);
                if (n == 0) {
                    printf("uart[%d] CMD client closed (fd=%d)\n", i, uart->cmd_client_fd);
                    set_state(&uart->sock_cmd_state, STATE_TCP_CLOSE, i, "CMD", uart->sock_cmd_port);
                } else if (n < 0) {
                    perror("client socket error (cmd)");
                    set_state(&uart->sock_cmd_state, STATE_TCP_CLOSE, i, "CMD", uart->sock_cmd_port);
                } else {
                    n = recv(uart->cmd_client_fd, cmd_tmp, sizeof(cmd_tmp), 0);
                    if (n > 0) {
                        uart_instances[i].cmd_count += n;
//                        printf("ua[%d] CMD %d all %lld bytes\n", i, n, uart_instances[i].cmd_count);
//                        set_state(&uart->sock_cmd_state, STATE_RW_DATA, i, "CMD", uart->sock_cmd_port);
                        handle_command(uart, uart->cmd_client_fd, cmd_tmp, n, i);
                    } else if (n == 0) {
                        printf("uart[%d] CMD client closed while reading (fd=%d)\n", i, uart->cmd_client_fd);
                        set_state(&uart->sock_cmd_state, STATE_TCP_CLOSE, i, "CMD", uart->sock_cmd_port);
                    } else {
                        perror("recv error on cmd client");
                        set_state(&uart->sock_cmd_state, STATE_TCP_CLOSE, i, "CMD", uart->sock_cmd_port);
                    }
                }
            }
            if (uart->sock_cmd_state == STATE_TCP_CLOSE) {
                if (uart->cmd_client_fd >= 0) {
                    close(uart->cmd_client_fd);
                    uart->cmd_client_fd = -1;
                }
                set_state(&uart->sock_cmd_state, STATE_TCP_OPEN, i, "CMD", uart->sock_cmd_port);
            }
        }
    }
}

// --------------------- sock_data 任务 ------------------------
void multi_tcp_data_servers_loop(int unused)
{
    int i;
    for (i = 0; i < NUM_PORTS; ++i) {
        uart_instances[i].data_client_fd = -1;
        uart_instances[i].sock_data_state = STATE_TCP_OPEN;
    }
    taskDelay(25);
    printf("tcp data loop start ... \n");
    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        int max_fd = 0;
        for (i = 0; i < NUM_PORTS; ++i) {
            if (uart_instances[i].sock_data > 0 && uart_instances[i].data_client_fd < 0) {
                FD_SET(uart_instances[i].sock_data, &rfds);
                if (uart_instances[i].sock_data > max_fd)
                    max_fd = uart_instances[i].sock_data;
            }
            if (uart_instances[i].data_client_fd >= 0) {
                FD_SET(uart_instances[i].data_client_fd, &rfds);
                if (uart_instances[i].data_client_fd > max_fd)
                    max_fd = uart_instances[i].data_client_fd;
            }
        }
        int ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select error (data)");
            taskDelay(5);
            continue;
        }
        for (i = 0; i < NUM_PORTS; ++i) {
            UART_Config_Params *uart = &uart_instances[i];
            if (uart->sock_data > 0 && uart->data_client_fd < 0 && FD_ISSET(uart->sock_data, &rfds)) {
                struct sockaddr_in caddr;
                int clen = sizeof(caddr);
                int client_fd = accept(uart->sock_data, (struct sockaddr*)&caddr, &clen);
                if (client_fd >= 0) {
                    uart->data_client_fd = client_fd;
                    uart_instances[i].data_count = 0;
                    set_state(&uart->sock_data_state, STATE_TCP_CONN, i, "DATA", uart->sock_data_port);
                    printf("uart[%d] sock_data (data_port %u) client connected, fd=%d\n",
                            i, uart->sock_data_port, client_fd);
                }
            }
            // 数据与断开
            if (uart->data_client_fd >= 0 && FD_ISSET(uart->data_client_fd, &rfds)) {
                int n = recv(uart->data_client_fd, sock_data_tmp, sizeof(sock_data_tmp), MSG_PEEK);
                if (n == 0) {
                    printf("uart[%d] DATA client closed (fd=%d)\n", i, uart->data_client_fd);
                    set_state(&uart->sock_data_state, STATE_TCP_CLOSE, i, "DATA", uart->sock_data_port);
                } else if (n < 0) {
                    perror("client socket error (data)");
                    set_state(&uart->sock_data_state, STATE_TCP_CLOSE, i, "DATA", uart->sock_data_port);
                } else {
                    n = recv(uart->data_client_fd, sock_data_tmp, sizeof(sock_data_tmp), 0);
                    if (n > 0) {
                        uart_instances[i].data_count += n;
                        uart_ring_buffer_enqueue(&uart->data_tx, sock_data_tmp, n);
//                        set_state(&uart->sock_data_state, STATE_RW_DATA, i, "DATA", uart->sock_data_port);
                        // printf("ua[%d] Data [%d]\n", i, n);
                        set_state(&uart->sock_data_state, STATE_TCP_CONN, i, "DATA", uart->sock_data_port);
                    } else if (n == 0) {
                        printf("uart[%d] DATA client closed while reading (fd=%d)\n", i, uart->data_client_fd);
                        set_state(&uart->sock_data_state, STATE_TCP_CLOSE, i, "DATA", uart->sock_data_port);
                    } else {
                        perror("recv error on data client");
                        set_state(&uart->sock_data_state, STATE_TCP_CLOSE, i, "DATA", uart->sock_data_port);
                    }
                }
            }
            if (uart->sock_data_state == STATE_TCP_CLOSE) {
                if (uart->data_client_fd >= 0) {
                    close(uart->data_client_fd);
                    uart->data_client_fd = -1;
                    ring_buffer_init(&uart->data_rx, uart->rx_buffer, BUFFERCOM_SIZE);
                    ring_buffer_init(&uart->data_tx, uart->tx_buffer, BUFFERCOM_SIZE);
                    printf("ua[%d] Data all [%lld]\n", i, uart_instances[i].data_count);
                }
                set_state(&uart->sock_data_state, STATE_TCP_OPEN, i, "DATA", uart->sock_data_port);
            }
        }
    }
}



// 动态计算建议tick延时
int calc_poll_delay_ticks(unsigned int baud_rate)
{
    /* 一字节时间 = 10/baud_rate (10bit/byte: start+8data+stop) */
    /* 动态：低速高延时，高速低延时，建议范围1ms~20ms */
    unsigned int min_ms = 1, max_ms = 20;
    unsigned int byte_time_us = baud_rate ? ((10*1000000U)/baud_rate) : 200000; // fallback: slow 
    unsigned int base_ms = (byte_time_us+999)/1000;    // 上取整，字节耗时多少ms
    if (base_ms < min_ms) base_ms=min_ms;
    if (base_ms > max_ms) base_ms=max_ms;
    // 系统tick
    int sys_tick = sysClkRateGet();
    int ticks = (base_ms * sys_tick + 999)/1000; // 上取整ms->tick
    if (ticks < 1) ticks = 1;
    return ticks;
}

// 多UART动态tx出队/发送任务
void multi_uart_tx_loop(int unused)
{
    int i;
    taskDelay(20);
    printf("uart_tx_loop running .... \n");
    while (1) {
        for (i = 0; i < NUM_PORTS; ++i) {
            UART_Config_Params *uart = &uart_instances[i];
            // 只有socket全部建立连接才发
//            if (uart->sock_cmd_state == STATE_TCP_CONN && uart->sock_data_state == STATE_TCP_CONN) {
            	if (uart->sock_data_state == STATE_TCP_CONN) {
                // 可选：确认data_tx缓冲区有数据
                size_t avail = uart->data_tx.head_index - uart->data_tx.tail_index;
                if (avail > 0) {
                    // 取数据（根据串口效率推荐一次最大发送量256字节）
                	if(axi16550_TxReady(i)){
                        ring_buffer_size_t n = uart_ring_buffer_dequeue(&uart->data_tx, uart_fpga_txbuf, sizeof(uart_fpga_txbuf));
    //                	printf("tail:%d head:%d \n ",uart_instances[i].data_tx.tail_index,uart_instances[i].data_tx.head_index);
                        if (n > 0) {
                            // 串口发送
                            int sent = axi16550Send(i, (uint8_t *)uart_fpga_txbuf, n);
                            // 可计数或错误处理
                            printf("[%d],uart[ %d ] : %d send n:%d... \n", __LINE__, i, sent,n);
                        }
                	}
                }
            }
            // 其它状态不发送
        }
        // 根据不同通道的串口波特率，选本轮最短poll
        int min_ticks = 20; // 最大上限（20ms）
        for (i=0; i<NUM_PORTS; ++i) {
            int t = calc_poll_delay_ticks(uart_instances[i].config.baud_rate);
            if (t < min_ticks) min_ticks = t;
        }
        if (min_ticks < UART_TX_TASK_MIN_DELAY) min_ticks = UART_TX_TASK_MIN_DELAY;
        taskDelay(msToTicks(min_ticks)); // 动态适应各通道，未建连优先轮询最小
    }
}

//---------  阶段一：高速poll UART收数据，写data_rx ----------
void multi_uart_rx_loop(int unused)
{
    int i;
    uint32_t rxlen;
    taskDelay(10);
    printf("multi_uart_rx_loop started.\n");
    while (1) {
        for (i=0; i<NUM_PORTS; ++i) {
            UART_Config_Params* uart = &uart_instances[i];
            if (uart->sock_cmd_state == STATE_TCP_CONN && uart->sock_data_state == STATE_TCP_CONN) 
            {
                do {
                    rxlen = sizeof(uart_fpga_rxbuf);
                    if (axi16550Recv(i, uart_fpga_rxbuf, &rxlen) == 0 && rxlen > 0) {
//                    	printf("[%d],rx[%d]:[%d]\n",__LINE__,i,rxlen);
                        uart_ring_buffer_enqueue(&uart->data_rx, (char*)uart_fpga_rxbuf, rxlen);
                        // 可统计接收字节等逻辑
                    }
                } while (rxlen > 0);
            }
        }
        // 动态delay, 各路最短
        int min_ticks = 5;
        for (i=0; i<NUM_PORTS; ++i) {
            int t = calc_poll_delay_ticks(uart_instances[i].config.baud_rate);
            if (t < min_ticks) min_ticks = t;
        }
        if (min_ticks < UART_RX_TASK_MIN_DELAY) min_ticks = UART_RX_TASK_MIN_DELAY;
//        printf("[%d],min_ticks[ %d ]. \n", __LINE__, min_ticks);
        taskDelay(msToTicks(min_ticks));
    }
}

//-------- 阶段二：批量dequeue转发到data_client_fd -------
void multi_uart_forward_loop(int unused)
{
    int i;
    size_t n;
    taskDelay(15);
    printf("multi_uart_forward_loop started.\n");
    while (1) {
        for (i = 0; i < NUM_PORTS; ++i) {
            UART_Config_Params *uart = &uart_instances[i];
            if (uart->sock_data_state == STATE_TCP_CONN &&
                uart->data_client_fd >= 0) {
                // 一次批量发送，提升效率
                n = uart_ring_buffer_dequeue(&uart->data_rx, net_tx_buf, sizeof(net_tx_buf));
                if (n > 0) {
                    int total = 0;
                    while (total < (int)n) {
//                      	printf("%d,sock[%d]:[%d]\n",__LINE__,i,n);
                        int sent = send(uart->data_client_fd, net_tx_buf + total, n - total, 0);
                        if (sent <= 0) break; // socket关闭/异常
                        total += sent;
                    }
                }
            }
        }
        // 动态delay，同上（主要服务于低波特率场景）
        int min_ticks = 20;
        for (i=0; i<NUM_PORTS; ++i) {
            int t = calc_poll_delay_ticks(uart_instances[i].config.baud_rate);
            if (t < min_ticks) min_ticks = t;
        }
        if (min_ticks < UART_RX_FORWARD_TASK_MIN_DELAY) min_ticks = UART_RX_FORWARD_TASK_MIN_DELAY;
//        printf("[%d],min_ticks[ %d ]. \n", __LINE__, min_ticks);
        taskDelay(msToTicks(min_ticks));
    }
}

void InitUartTask(UART_Config_Params *uart_instances, int num_ports) {
    int ret = -1;
	for (int i = 0; i < num_ports; i++) {
		// 初始化端口配置
		uart_instances[i].sock_cmd_port = portcmd_array[i];
		uart_instances[i].sock_data_port = portdata_array[i];

        uart_instances[i].sock_cmd_state = STATE_INIT;
        uart_instances[i].sock_data_state = STATE_INIT;
        uart_instances[i].cmd_client_fd = -1;
        uart_instances[i].data_client_fd = -1;

        uart_instances[i].cmd_count = 0;
        uart_instances[i].data_count = 0;

        ring_buffer_init(&uart_instances[i].data_rx, uart_instances[i].rx_buffer, BUFFERCOM_SIZE);
        ring_buffer_init(&uart_instances[i].data_tx, uart_instances[i].tx_buffer, BUFFERCOM_SIZE);

		// 创建消息队列
		uart_instances[i].msg_queue = msgQCreate(16, 256, MSG_Q_FIFO);
		if (uart_instances[i].msg_queue == NULL) {
			perror("[UART] msgQCreate failed");
			continue;
		}

        /*  cmd socket init  */
        ret = create_tcp_server_socket(&uart_instances[i].sock_cmd);
        if(ret < 0 ){
            printf("Failed to create sock_cmd.\n"); 
            return -1;
        }

        if(bind_tcp_server_socket(uart_instances[i].sock_cmd,uart_instances[i].sock_cmd_port)<0){
            printf("Failed to bind socket for port %d\n", uart_instances[i].sock_cmd_port);
            goto exit1;
        }

        if (listen_tcp_server_socket(uart_instances[i].sock_cmd) < 0) {
            printf("Failed to listen socket for port %d\n", uart_instances[i].sock_cmd_port);
            goto exit1;
        }

        /*  data socket init  */
        ret = create_tcp_server_socket(&uart_instances[i].sock_data);
        if(ret < 0 ){
            printf("Failed to create sock_data.\n"); 
            goto exit2;
        }

        if(bind_tcp_server_socket(uart_instances[i].sock_data,uart_instances[i].sock_data_port)<0){
            printf("Failed to bind socket for port %d\n", uart_instances[i].sock_data_port);
            goto exit2;
        }

        if (listen_tcp_server_socket(uart_instances[i].sock_data) < 0) {
            printf("Failed to listen socket for port %d\n", uart_instances[i].sock_data_port);

            goto exit2;
        }

        continue;

        exit2:
            close(uart_instances[i].sock_data);
        exit1:
            close(uart_instances[i].sock_cmd);
	}

    int tid = taskSpawn("multi_tcp_cmd_servers_loop", TCP_SERVER_TASK_PRIO, 0, TCP_SERVER_TASK_STACK, 
        (FUNCPTR)multi_tcp_cmd_servers_loop,  0, 0,0,0,0,0,0,0,0,0);


    if (tid == ERROR) {
        perror("multi_tcp_cmd_loop failed");
        goto exit;
    }

    tid = taskSpawn("multi_tcp_data_servers_loop", TCP_SERVER_TASK_PRIO, 0, TCP_SERVER_TASK_STACK, 
        (FUNCPTR)multi_tcp_data_servers_loop,  0, 0,0,0,0,0,0,0,0,0);
        if (tid == ERROR) {
            perror("multi_tcp_data_servers_loop failed");
            goto exit;
        }

    tid =  taskSpawn("uartTxLoop",UART_TX_TASK_PRIO,0,UART_TASK_STACK,(FUNCPTR)multi_uart_tx_loop,0,0,0,0,0,0,0,0,0,0);
    if (tid == ERROR) {
        perror("uartTxLoop failed");
        goto exit;
    }

    tid =  taskSpawn("uartRxLoop",  UART_RX_TASK_PRIO,0,UART_TASK_STACK,(FUNCPTR)multi_uart_rx_loop, 0,0,0,0,0,0,0,0,0,0);
    if (tid == ERROR) {
        perror("uartRxLoop failed");
        goto exit;
    }
    tid =  taskSpawn("uartNetFwd", UART_RX_FORWARD_TASK_PRIO,0,UART_TASK_STACK,(FUNCPTR)multi_uart_forward_loop,0,0,0,0,0,0,0,0,0,0);
    if (tid == ERROR) {
        perror("uartNetFwd failed");
        goto exit;
    }
    goto success;
exit:
    perror("Init failed ... \n");
success:
    printf("Init Success ... \n");
}


void startUartServer() {
	taskSpawn("InitUartTask", 90, 0, 40000, 
			(FUNCPTR)InitUartTask,  (long)(void *)uart_instances,NUM_PORTS,0,0,0,0,0,0,0,0);
}
