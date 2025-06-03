#include "serial_bridge.h"
#include "net_bridge.h"
#include "axi16550.h"

UART_Config_Params uart_instances[NUM_PORTS];
uart_led_stat_t uart_led_stats[NUM_PORTS];
static int sys_tick = 0;

#define UART_FPGA_RX_BUFFER             (2048)
#define UART_FPGA_TX_BUFFER             (256+16)
static char uart_fpga_rxbuf[UART_FPGA_RX_BUFFER];
static char uart_fpga_txbuf[NUM_PORTS][UART_FPGA_TX_BUFFER];
static char uart_fpga_txbuf_tmp[UART_FPGA_TX_BUFFER];

/* Time-related helper functions */
void timeStart(TimeCounter *tc)
{
    tc->startTick = tickGet();
    tc->clkRate   = sysClkRateGet();
}

void timeEnd(TimeCounter *tc, UINT32 *pTicks, UINT32 *pMs)
{
    UINT32 endTick = tickGet();
    *pTicks = endTick - tc->startTick;          // 计算 tick 差
    *pMs    = (*pTicks * 1000) / tc->clkRate;  // 转换为毫秒
}

int msToTicks(int ms)
{
    return (ms * sysClkRateGet() + 999) / 1000;
}

/* LED control functions */
void txled(int i, int action)
{
    // 确保传入的索引在范围内
    if (i < 0 || i > 15) return;

    uint32_t reg_address = 0x130 + (i * 4);  // 计算对应的寄存器地址
    uint32_t value = (action == 1) ? 1 : 0;  // 根据动作设置LED状态

    PL_AXI_WriteReg(PL_AXI_BASE, reg_address, value);
}

void rxled(int i, int action)
{
    // 确保传入的索引在范围内
    if (i < 0 || i > 15) return;

    uint32_t reg_address = 0x230 + (i * 4);  // 计算对应的寄存器地址
    uint32_t value = (action == 1) ? 1 : 0;  // 根据动作设置LED状态

    PL_AXI_WriteReg(PL_AXI_BASE, reg_address, value);
}

void Portled(int i, int action) {
    if (i < 0 || i > 15) return;

    uint32_t reg_address = 0x30 + (i * 4);  
    uint32_t value = (action == 1) ? 1 : 0;  

    PL_AXI_WriteReg(PL_AXI_BASE, reg_address, value);
}

/* Ring buffer functions */
void uart_ring_buffer_enqueue(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size)
{
    if (buffer == NULL || data == NULL || size == 0)
        return;
    ring_buffer_queue_arr(buffer, data, size);
}

ring_buffer_size_t uart_ring_buffer_dequeue(ring_buffer_t *buffer, char *data, ring_buffer_size_t len)
{
    if (buffer == NULL || data == NULL || len == 0)
        return 0;
    return ring_buffer_dequeue_arr(buffer, data, len);
}

/* Calculate sample period based on baud rate */
uint16_t calc_sample_period(uint32_t baudrate, uint32_t sys_tick_hz, int task_delay_tick)
{
    if (baudrate == 0) baudrate = 9600;
    // 采样时间 = 传输 SAMPLE_BYTES 字节时间
    uint32_t period_ticks = (SAMPLE_BYTES * 10 * sys_tick_hz + baudrate - 1) / baudrate; // 向上取整
    if (period_ticks < 1) period_ticks = 1;

    // 保证不小于任务延时，避免采样周期小于任务周期导致逻辑异常
    if (period_ticks < (uint32_t)task_delay_tick)
        period_ticks = task_delay_tick;

    return (uint16_t)period_ticks;
}

/* Update LED status based on traffic */
void update_led_state(uart_led_stat_t *stat, int is_tx)
{
    uint16_t *count = is_tx ? &stat->tx_count : &stat->rx_count;
    uint16_t *sample_tick = is_tx ? &stat->sample_tick_cnt_tx : &stat->sample_tick_cnt_rx;
    uint16_t sample_period = is_tx ? stat->sample_period_ticks_tx : stat->sample_period_ticks_rx;
    uint8_t *led_state = is_tx ? &stat->tx_led_state : &stat->rx_led_state;

    (*sample_tick)++;
    if ((*sample_tick) >= sample_period)
    {
        // 利用率百分比
        uint16_t util_percent = (*count) * 100 / SAMPLE_BYTES;

        if (util_percent >= 70)
        {
            *led_state = 1; // 高负载常亮
        }
        else if (util_percent >= 10)
        {
            // 快闪，状态取反
            *led_state = (*led_state) ? 0 : 1;
        }
        else if (util_percent > 0)
        {
            // 慢闪，周期为两采样周期，前半亮后半灭
            *led_state = ((*sample_tick) % (2 * sample_period)) < sample_period ? 1 : 0;
        }
        else
        {
            // 无负载灭灯
            *led_state = 0;
        }

        // 清空计数，重置采样计时
        *count = 0;
        *sample_tick = 0;
    }
}

/* Calculate transmit buffer limit based on baud rate */
int calc_tx_buffer_limit(uint32_t baudrate, int tick_rate)
{
    if (baudrate == 0)
        baudrate = 9600;  // 安全兜底

    float tick_sec = 1.0f / tick_rate;
    float period_sec = tick_sec * UART_TX_TASK_MIN_DELAY;

    // 波特率字节数，安全倍数设为2倍（可调）
    float safety_factor = 1.5f;

    int max_bytes = (int)((baudrate * period_sec) / 10 * safety_factor);

    // 取最大1，避免0
    if (max_bytes < 1)
        max_bytes = 1;

    // 不超过环形缓冲最大区大小
    if (max_bytes > UART_FPGA_TX_BUFFER)
        max_bytes = UART_FPGA_TX_BUFFER;

    return max_bytes;
}

/* Calculate polling delay based on baud rate */
int calc_poll_delay_ticks(unsigned int baud_rate)
{
    /* 一字节时间 = 10/baud_rate (10bit/byte: start+8data+stop) */
    /* 动态：低速高延时，高速低延时，建议范围1ms~20ms */
    unsigned int min_ms = 1, max_ms = 20;
    unsigned int byte_time_us = baud_rate ? ((10 * 1000000U) / baud_rate) : 200000; // fallback: slow
    unsigned int base_ms = (byte_time_us + 999) / 1000; // 上取整，字节耗时多少ms
    if (base_ms < min_ms) base_ms = min_ms;
    if (base_ms > max_ms) base_ms = max_ms;
    // 系统tick
    int sys_tick = sysClkRateGet();
    int ticks = (base_ms * sys_tick + 999) / 1000; // 上取整ms->tick
    if (ticks < 1) ticks = 1;
    return ticks;
}

/* UART transmit task */
void multi_uart_tx_loop(int unused)
{
    int i;
    taskDelay(20);
    printf("uart_tx_loop running .... \n");
    TimeCounter tc;
    UINT32 ticks, ms;
    uint32_t sys_tick_hz = sysClkRateGet();
    ring_buffer_size_t rx_ready[NUM_PORTS];
    for (i = 0; i < NUM_PORTS; ++i) rx_ready[i] = 0;

    for (i = 0; i < NUM_PORTS; i++)
    {
        uart_led_stats[i].sample_period_ticks_tx = calc_sample_period(uart_instances[i].config.baud_rate, sys_tick_hz, UART_TX_TASK_MIN_DELAY);
        uart_led_stats[i].tx_count = 0;
        uart_led_stats[i].sample_tick_cnt_tx = 0;
        uart_led_stats[i].tx_led_state = 0;
    }

    while (1)
    {
        for (i = 0; i < NUM_PORTS; ++i)
        {
            UART_Config_Params *uart = &uart_instances[i];
            // 只有socket全部建立连接才发
            if (uart->sock_cmd_state == STATE_TCP_CONN && uart->sock_data_state == STATE_TCP_CONN)
            {
                if (0 == rx_ready[i])
                {
                    rx_ready[i] = uart_ring_buffer_dequeue(&uart->data_tx, &uart_fpga_txbuf[i][0], uart->tx_buffer_limit);
                }
                if (rx_ready[i] > 0)
                {
                    uart_led_stats[i].tx_count += rx_ready[i];
                    timeStart(&tc);
                    if (uart->config.baud_rate <= 9600)
                    {
                        axi16550Send(i, (uint8_t *)&uart_fpga_txbuf[i][0], rx_ready[i]);
                    }
                    else
                    {
                        axi16550Send(i, (uint8_t *)&uart_fpga_txbuf[i][0], rx_ready[i]);
                    }

                    timeEnd(&tc, &ticks, &ms);
                    rx_ready[i] = 0;
                }
            }
            update_led_state(&uart_led_stats[i], 1);
            txled(i, uart_led_stats[i].tx_led_state);
        }
        taskDelay(UART_TX_TASK_MIN_DELAY);
    }
}

/* UART receive task */
void multi_uart_rx_loop(int unused)
{
    int i;
    uint32_t rxlen;
    uint32_t sys_tick_hz = sysClkRateGet();
    for (i = 0; i < NUM_PORTS; i++)
    {
        uart_led_stats[i].sample_period_ticks_rx = calc_sample_period(uart_instances[i].config.baud_rate, sys_tick_hz, UART_RX_TASK_MIN_DELAY);
        uart_led_stats[i].rx_count = 0;
        uart_led_stats[i].sample_tick_cnt_rx = 0;
        uart_led_stats[i].rx_led_state = 0;
    }
    taskDelay(10);
    printf("multi_uart_rx_loop started.\n");
    while (1)
    {
        for (i = 0; i < NUM_PORTS; ++i)
        {
            UART_Config_Params *uart = &uart_instances[i];
            if (uart->sock_cmd_state == STATE_TCP_CONN && uart->sock_data_state == STATE_TCP_CONN)
            {
                do
                {
                    rxlen = sizeof(uart_fpga_rxbuf);
                    if (axi16550Recv(i, uart_fpga_rxbuf, &rxlen) == 0 && rxlen > 0)
                    {
                        uart_led_stats[i].rx_count += rxlen;
                        uart_ring_buffer_enqueue(&uart->data_rx, (char *)uart_fpga_rxbuf, rxlen);
                    }
                }
                while (rxlen > 0);
            }
            // 周期更新RX LED状态
            update_led_state(&uart_led_stats[i], 0);
            rxled(i, uart_led_stats[i].rx_led_state);
        }
        taskDelay(UART_RX_TASK_MIN_DELAY);
    }
}

/* Initialize all UART ports and start tasks */
void InitUartTask(UART_Config_Params *uart_instances, int num_ports)
{
    int ret = -1;
    int i;
    for (i = 0; i < num_ports; i++)
    {
        // 初始化端口配置
        uart_instances[i].sock_cmd_port = portcmd_array[i];
        uart_instances[i].sock_data_port = portdata_array[i];

        uart_instances[i].sock_cmd_state = STATE_INIT;
        uart_instances[i].sock_data_state = STATE_INIT;
        uart_instances[i].cmd_client_fd = -1;
        uart_instances[i].data_client_fd = -1;

        uart_instances[i].cmd_count = 0;
        uart_instances[i].data_count = 0;

        ring_buffer_init(&uart_instances[i].data_rx, uart_instances[i].rx_buffer, BUFFERCOM_SIZE_RX);
        ring_buffer_init(&uart_instances[i].data_tx, uart_instances[i].tx_buffer, BUFFERCOM_SIZE_TX);

        // 创建消息队列
        uart_instances[i].msg_queue = msgQCreate(16, 256, MSG_Q_FIFO);
        if (uart_instances[i].msg_queue == NULL)
        {
            perror("[UART] msgQCreate failed");
            continue;
        }

        /*  cmd socket init  */
        ret = create_tcp_server_socket(&uart_instances[i].sock_cmd);
        if (ret < 0)
        {
            printf("Failed to create sock_cmd.\n");
            return -1;
        }

        if (bind_tcp_server_socket(uart_instances[i].sock_cmd, uart_instances[i].sock_cmd_port) < 0)
        {
            printf("Failed to bind socket for port %d\n", uart_instances[i].sock_cmd_port);
            goto exit1;
        }

        if (listen_tcp_server_socket(uart_instances[i].sock_cmd) < 0)
        {
            printf("Failed to listen socket for port %d\n", uart_instances[i].sock_cmd_port);
            goto exit1;
        }

        /*  data socket init  */
        ret = create_tcp_server_socket(&uart_instances[i].sock_data);
        if (ret < 0)
        {
            printf("Failed to create sock_data.\n");
            goto exit2;
        }

        if (bind_tcp_server_socket(uart_instances[i].sock_data, uart_instances[i].sock_data_port) < 0)
        {
            printf("Failed to bind socket for port %d\n", uart_instances[i].sock_data_port);
            goto exit2;
        }

        if (listen_tcp_server_socket(uart_instances[i].sock_data) < 0)
        {
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
            (FUNCPTR)multi_tcp_cmd_servers_loop, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (tid == ERROR)
    {
        perror("multi_tcp_cmd_loop failed");
        goto exit;
    }

    tid = taskSpawn("multi_tcp_data_servers_loop", TCP_SERVER_TASK_PRIO, 0, TCP_SERVER_TASK_STACK,
            (FUNCPTR)multi_tcp_data_servers_loop, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (tid == ERROR)
    {
        perror("multi_tcp_data_servers_loop failed");
        goto exit;
    }

    tid = taskSpawn("uartTxLoop", UART_TX_TASK_PRIO, 0, UART_TASK_STACK, 
            (FUNCPTR)multi_uart_tx_loop, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (tid == ERROR)
    {
        perror("uartTxLoop failed");
        goto exit;
    }

    tid = taskSpawn("uartRxLoop", UART_RX_TASK_PRIO, 0, UART_TASK_STACK, 
            (FUNCPTR)multi_uart_rx_loop, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (tid == ERROR)
    {
        perror("uartRxLoop failed");
        goto exit;
    }
    
    tid = taskSpawn("uartNetFwd", UART_RX_FORWARD_TASK_PRIO, 0, UART_TASK_STACK, 
            (FUNCPTR)multi_uart_forward_loop, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (tid == ERROR)
    {
        perror("uartNetFwd failed");
        goto exit;
    }
    
    // 启动心跳任务
//    tid = taskSpawn("heartbeatSendTask", 90, 0, 40000,
//            (FUNCPTR)heartbeat_send_task, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (tid == ERROR)
    {
        perror("heartbeatSendTask failed");
        goto exit;
    }
    
    goto success;
    
exit:
    perror("Init failed ... \n");
success:
    printf("Init Success ... \n");
}

/* Start the UART server */
void startUartServer()
{
    taskSpawn("InitUartTask", 90, 0, 40000,
            (FUNCPTR)InitUartTask, (long)(void *)uart_instances, NUM_PORTS, 0, 0, 0, 0, 0, 0, 0, 0);
}

