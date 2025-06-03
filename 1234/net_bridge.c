
#include "net_bridge.h"
#include "serial_bridge.h"
#include "net_cmd.h"
#include "axi16550.h"


const char *STATE_NAMES[] =
{
    "STATE_INIT",        // 0
    "STATE_TCP_OPEN",    // 1
    "STATE_TCP_CONN",    // 2
    "STATE_TCP_CLOSE",   // 3
    "STATE_TCP_WAIT",    // 4
    "STATE_RW_DATA",     // 5
};

const int portdata_array[] = {950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960, 961, 962, 963, 964, 965};
const int portcmd_array[] = {966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981};

#define SOCK_DATA_BUFFER        (8192*2*2)
static char sock_data_tmp[SOCK_DATA_BUFFER];

#define SOCK_cmd_BUFFER        (256)
static char cmd_tmp[SOCK_cmd_BUFFER];

#define NET_BUFFER            (8192*2*2)
static char net_tx_buf[NET_BUFFER];

/* Create TCP server socket */
int create_tcp_server_socket(int *sock_fd)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket creation failed");
        return -1;
    }
    *sock_fd = sock;
    return 1;
}

/* Bind TCP server socket to port */
int bind_tcp_server_socket(int sock_fd, int port)
{
    struct sockaddr_in server_addr;
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    DEBUG_PRINT("bind sock: %d  port %d \n", sock_fd, port);
    int ret = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        perror("bind failed");
        return -1;
    }
    return 0;
}

/* Set socket to listen state */
int listen_tcp_server_socket(int sock_fd)
{
    DEBUG_PRINT("listen sock: %d \n", sock_fd);
    int ret = listen(sock_fd, BACKLOG);
    if (ret < 0)
    {
        perror("listen failed");
        return -1;
    }
    return 0;
}

/* Helper function to change state with logging */
void set_state(uint8_t *var, sock_state_enum new_state, int idx, const char *side, uint16_t port)
{
    if (*var != new_state)
    {
        // printf("[%s %d port %u] State: %s -> %s\n",
        //     side, idx, port, STATE_NAMES[*var], STATE_NAMES[new_state]);
        *var = new_state;
    }
}

/* Send information about UART parameters */
void uart_info_send(uint8_t i)
{
    unsigned char send_buf[4] = {0x26, 0x00, 0x00, 0x81};
    int send_len = sizeof(send_buf);
    if (send(uart_instances[i].cmd_client_fd, send_buf, send_len, 0) != send_len)
    {
        perror("send failed");
    }
}

/* Check for timed-out CMD connections */
void check_tcp_cmd_timeouts(void)
{
    int i;
    ULONG now = tickGet();
    ULONG timeout_ticks = sysClkRateGet() * 60;  // 60秒空闲断开

    for (i = 0; i < NUM_PORTS; ++i)
    {
        UART_Config_Params *uart = &uart_instances[i];

        if (uart->cmd_client_fd >= 0)
        {
            if ((now - uart->last_activity_time) > timeout_ticks)
            {
                DEBUG_PRINT("uart[%d] cmd client timeout, closing fd=%d", i, uart->cmd_client_fd);
                close(uart->cmd_client_fd);
                uart->cmd_client_fd = -1;
                uart->sock_cmd_state = STATE_TCP_OPEN;
            }
        }
    }
}

/* Check for timed-out DATA connections */
void check_tcp_data_timeouts(void)
{
    int i;
    ULONG now = tickGet();
    ULONG timeout_ticks = sysClkRateGet() * 60;   // 60秒超时

    for (i = 0; i < NUM_PORTS; i++)
    {
        UART_Config_Params *uart = &uart_instances[i];

        if (uart->data_client_fd >= 0)
        {
            if ((now - uart->last_activity_time) > timeout_ticks)
            {
                DEBUG_PRINT("uart[%d] data client timeout, closing fd=%d", i, uart->data_client_fd);
                close(uart->data_client_fd);
                uart->data_client_fd = -1;
                uart->sock_data_state = STATE_TCP_OPEN;
            }
        }
    }
}

/* Main task for command channel */
void multi_tcp_cmd_servers_loop(int unused)
{
    int i;

    // 初始化每个串口cmd连接状态和fd
    for (i = 0; i < NUM_PORTS; ++i)
    {
        uart_instances[i].cmd_client_fd = -1;
        uart_instances[i].sock_cmd_state = STATE_TCP_OPEN;
        uart_instances[i].last_activity_time = tickGet();
        uart_instances[i].cmd_count = 0;
    }

    taskDelay(30);
    printf("tcp cmd loop start ... sys_tick: %d\n", sysClkRateGet());

    while (1)
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        int max_fd = -1;

        // 注册监听和客户端FD
        for (i = 0; i < NUM_PORTS; ++i)
        {
            UART_Config_Params *uart = &uart_instances[i];

            if (uart->sock_cmd > 0 && uart->cmd_client_fd < 0)
            {
                FD_SET(uart->sock_cmd, &rfds);
                if (uart->sock_cmd > max_fd)
                    max_fd = uart->sock_cmd;
            }

            if (uart->cmd_client_fd >= 0)
            {
                FD_SET(uart->cmd_client_fd, &rfds);
                if (uart->cmd_client_fd > max_fd)
                    max_fd = uart->cmd_client_fd;
            }
        }

        if (max_fd < 0)
        {
            // 无有效fd，避免空循环
            taskDelay(10);
            continue;
        }

        // select超时设置为1秒，防止任务长时间阻塞
        struct timeval select_timeout;
        select_timeout.tv_sec = 1;
        select_timeout.tv_usec = 0;

        int ret = select(max_fd + 1, &rfds, NULL, NULL, &select_timeout);
        if (ret < 0)
        {
            perror("select error (cmd)");
            taskDelay(5);
            continue;
        }
        else if (ret == 0)
        {
            // 超时，检查空闲连接，断开长时间无响应连接
            check_tcp_cmd_timeouts();
            continue;
        }

        for (i = 0; i < NUM_PORTS; ++i)
        {
            UART_Config_Params *uart = &uart_instances[i];

            // 监听socket就绪，接收新链路
            if (uart->sock_cmd > 0 && uart->cmd_client_fd < 0 && FD_ISSET(uart->sock_cmd, &rfds))
            {
                struct sockaddr_in caddr;
                socklen_t clen = sizeof(caddr);
                int client_fd = accept(uart->sock_cmd, (struct sockaddr *)&caddr, &clen);
                if (client_fd >= 0)
                {
                    // 设置非阻塞
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

#ifdef TCP_KEEPIDLE
                    int idle = 1;
                    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
#endif
#ifdef TCP_KEEPINTVL
                    int interval = 1;
                    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
#endif
#ifdef TCP_KEEPCNT
                    int maxpkt = 3;
                    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(maxpkt));
#endif

                    uart->cmd_client_fd = client_fd;
                    uart->last_activity_time = tickGet();
                    uart->cmd_count = 0;
                    uart->sock_cmd_state = STATE_TCP_CONN;
                    uart_info_send(i);
                    DEBUG_PRINT("uart[%d] cmd port %u client connected, fd=%d", i, uart->sock_cmd_port, client_fd);
                }
                else
                {
                    perror("accept failed (cmd)");
                }
            }

            // 客户端socket可读，读取数据
            if (uart->cmd_client_fd >= 0 && FD_ISSET(uart->cmd_client_fd, &rfds))
            {
                int n = recv(uart->cmd_client_fd, cmd_tmp, sizeof(cmd_tmp), 0);
                if (n > 0)
                {
                    uart->last_activity_time = tickGet();
                    uart->cmd_count += n;
                    handle_command(uart, uart->cmd_client_fd, cmd_tmp, n, i);

                    if (cmd_tmp[0] == ASPP_CMD_PORT_INIT)
                    {
                        uart->tx_buffer_limit = calc_tx_buffer_limit(uart->config.baud_rate, sysClkRateGet());
                        DEBUG_PRINT("uart[%d] tx limit updated to %d", i, uart->tx_buffer_limit);
                    }
                }
                else if (n == 0)
                {
                    // 客户端正常关闭
                    DEBUG_PRINT("uart[%d] CMD client closed fd=%d", i, uart->cmd_client_fd);
                    close(uart->cmd_client_fd);
                    uart->cmd_client_fd = -1;
                    uart->sock_cmd_state = STATE_TCP_OPEN;
                    uart->cmd_count = 0;
                    uart->last_activity_time = 0;
                }
                else
                {
                    int err = errno;
                    if (err == EAGAIN || err == EWOULDBLOCK)
                    {
                        // 非阻塞无数据，忽略
                    }
                    else if (err == EINTR)
                    {
                        // 被信号打断，安全重试
                    }
                    else
                    {
                        // 其他错误立即关闭连接
                        DEBUG_PRINT("uart[%d] recv error fd=%d errno=%d (%s), closing connection", i, uart->cmd_client_fd, err, strerror(err));
                        close(uart->cmd_client_fd);
                        uart->cmd_client_fd = -1;
                        uart->sock_cmd_state = STATE_TCP_OPEN;
                        uart->cmd_count = 0;
                        uart->last_activity_time = 0;
                    }
                }
            }
        }

        taskDelay(TCP_TASK_MIN_DELAY);
    }
}

/* Main task for data channel */
void multi_tcp_data_servers_loop(int unused)
{
    int i;

    // 每通道初始化
    for (i = 0; i < NUM_PORTS; ++i)
    {
        uart_instances[i].data_client_fd = -1;
        uart_instances[i].sock_data_state = STATE_TCP_OPEN;
        uart_instances[i].last_activity_time = time(NULL);
    }

    taskDelay(25);

    DEBUG_PRINT("tcp data loop start ...\n");

    while (1)
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        int max_fd = -1;

        // 监听所有处于监听态的sock_data和已连接fd
        for (i = 0; i < NUM_PORTS; ++i)
        {
            UART_Config_Params *uart = &uart_instances[i];
            if (uart->sock_data > 0 && uart->data_client_fd < 0)
            {
                FD_SET(uart->sock_data, &rfds);
                if (uart->sock_data > max_fd)
                    max_fd = uart->sock_data;
            }
            if (uart->data_client_fd >= 0)
            {
                FD_SET(uart->data_client_fd, &rfds);
                if (uart->data_client_fd > max_fd)
                    max_fd = uart->data_client_fd;
            }
        }

        if (max_fd < 0)
        {
            // 没有有效socket，稍作休眠继续
            taskDelay(10);
            continue;
        }

        // 设置select超时，避免永久阻塞
        struct timeval select_timeout;
        select_timeout.tv_sec = 1;    // 1秒超时
        select_timeout.tv_usec = 0;

        // 永久阻塞等待I/O事件，有事件立刻处理，CPU期限内最优
        int ret = select(max_fd + 1, &rfds, NULL, NULL, &select_timeout);
        if (ret < 0)
        {
            perror("select error (data)");
            taskDelay(5);
            continue;
        }
        else if (ret == 0)
        {
            // 超时无事件，检查死连接
            check_tcp_data_timeouts();
            continue;
        }

        for (i = 0; i < NUM_PORTS; ++i)
        {
            UART_Config_Params *uart = &uart_instances[i];
            // 1) 监听socket就绪，接受新连接
            if (uart->sock_data > 0 && uart->data_client_fd < 0 && FD_ISSET(uart->sock_data, &rfds))
            {
                struct sockaddr_in caddr;
                socklen_t clen = sizeof(caddr);
                axi16550FIFOInit(i);
                ring_buffer_init(&uart->data_rx, uart->rx_buffer, BUFFERCOM_SIZE_RX);
                ring_buffer_init(&uart->data_tx, uart->tx_buffer, BUFFERCOM_SIZE_TX);
                int client_fd = accept(uart->sock_data, (struct sockaddr *)&caddr, &clen);
                if (client_fd >= 0)
                {
                    uart->data_client_fd = client_fd;
                    uart->data_count = 0;
                    uart->last_activity_time = tickGet();
                    uart->sock_data_state = STATE_TCP_CONN;
                    DEBUG_PRINT("uart[%d] sock_data port %u client connected, fd=%d", i, uart->sock_data_port, client_fd);
                    //add port LED set    
                    Portled(i, 1);
                    // 设置非阻塞并启用TCP keepalive
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

#ifdef TCP_KEEPIDLE
                    int idle = 1;
                    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
#endif
#ifdef TCP_KEEPINTVL
                    int interval = 1;
                    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
#endif
#ifdef TCP_KEEPCNT
                    int maxpkt = 3;
                    setsockopt(client_fd, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(maxpkt));
#endif
                }
            }

            // 2. 数据接收与异常断开
            if (uart->data_client_fd >= 0 && FD_ISSET(uart->data_client_fd, &rfds))
            {
                int n = recv(uart->data_client_fd, sock_data_tmp, sizeof(sock_data_tmp), 0);
                if (n > 0)
                {
                    uart->last_activity_time = tickGet();
                    uart->data_count += n;

                    if (ring_buffer_is_full(&uart->data_tx))
                    {
                        DEBUG_PRINT("uart[%d] data_tx ring buffer full, closing connection fd=%d", i, uart->data_client_fd);
                        close(uart->data_client_fd);
                        uart->data_client_fd = -1;
                        uart->sock_data_state = STATE_TCP_OPEN;

                        ring_buffer_init(&uart->data_rx, uart->rx_buffer, BUFFERCOM_SIZE_RX);
                        ring_buffer_init(&uart->data_tx, uart->tx_buffer, BUFFERCOM_SIZE_TX);
                        continue;  // 跳过本次循环处理，避免后续逻辑访问无效fd
                    }

                    uart_ring_buffer_enqueue(&uart->data_tx, sock_data_tmp, n);
                    uart->sock_data_state = STATE_TCP_CONN;
                }
                else if (n == 0)
                {
                    DEBUG_PRINT("uart[%d] DATA client closed fd=%d", i, uart->data_client_fd);
                    close(uart->data_client_fd);
                    uart->data_client_fd = -1;
                    uart->sock_data_state = STATE_TCP_OPEN;
                    //Port LED Clr
                    Portled(i, 0);
                    ring_buffer_init(&uart->data_rx, uart->rx_buffer, BUFFERCOM_SIZE_RX);
                    ring_buffer_init(&uart->data_tx, uart->tx_buffer, BUFFERCOM_SIZE_TX);
                }
                else
                {
                    int err = errno;
                    if (err == EAGAIN || err == EWOULDBLOCK)
                    {
                        // 非阻塞无数据，继续
                    }
                    else if (err == EINTR)
                    {
                        // 被信号中断，重试
                    }
                    else
                    {
                        DEBUG_PRINT("uart[%d] recv error fd=%d errno=%d (%s), closing connection", i, uart->data_client_fd, err, strerror(err));
                        close(uart->data_client_fd);
                        uart->data_client_fd = -1;
                        uart->sock_data_state = STATE_TCP_OPEN;

                        ring_buffer_init(&uart->data_rx, uart->rx_buffer, BUFFERCOM_SIZE_RX);
                        ring_buffer_init(&uart->data_tx, uart->tx_buffer, BUFFERCOM_SIZE_TX);
                    }
                }
            }
        }  // end for ports

        // 每轮循环末小延时，防止CPU占满
        taskDelay(TCP_TASK_MIN_DELAY);
    }  // end while
}

/* Heartbeat task */
void heartbeat_send_task()
{
    while (1)
    {
        int i;
        for (i = 0; i < NUM_PORTS; ++i)
        {
            UART_Config_Params *uart = &uart_instances[i];
            if (uart->cmd_client_fd >= 0)
            {
//                usart_report_hearbeat(uart->cmd_client_fd, NULL, 0);
            }
        }
        taskDelay(msToTicks(6000)); // 每60秒发送一次心跳包
    }
}

/* UART forwarding task - from buffer to network */
void multi_uart_forward_loop(int unused)
{
    int i;
    size_t n;
    taskDelay(15);
    printf("multi_uart_forward_loop started.\n");
    while (1)
    {
        for (i = 0; i < NUM_PORTS; ++i)
        {
            UART_Config_Params *uart = &uart_instances[i];
            if (uart->sock_data_state == STATE_TCP_CONN &&
                    uart->data_client_fd >= 0)
            {
                // 一次批量发送，提升效率
                n = uart_ring_buffer_dequeue(&uart->data_rx, net_tx_buf, sizeof(net_tx_buf));
                if (n > 0)
                {
                    int total = 0;
                    while (total < (int)n)
                    {
                        int sent = send(uart->data_client_fd, net_tx_buf + total, n - total, 0);
                        if (sent <= 0) break; // socket关闭/异常
                        total += sent;
                    }
                }
            }
        }
        taskDelay(UART_RX_FORWARD_TASK_MIN_DELAY);
    }
}
