#ifndef COMMON_H_
#define COMMON_H_

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
#include <netinet/in.h>

#include "ringbuffer.h"

#define BUFFERCOM_SIZE_RX  (65536*2*2)
#define BUFFERCOM_SIZE_TX  (65536*2*2)
#define NUM_PORTS       16
#define UART_HW_FIFO_SIZE               (4096)

#define BACKLOG   256
#define LED_ON  1
#define LED_OFF 0

#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
        do { \
            printf("[DEBUG] %d:%s(): " fmt "\n", \
                    __LINE__, __func__, ##__VA_ARGS__); \
        } while(0)
#else
#define DEBUG_PRINT(fmt, ...) do {} while(0)  // 无操作
#endif

/* State enumeration for TCP connections */
typedef enum
{
    STATE_INIT = 0,
    STATE_TCP_OPEN,
    STATE_TCP_CONN,
    STATE_TCP_CLOSE,
    STATE_TCP_WAIT,
    STATE_RW_DATA,
    STATE_MAX
} sock_state_enum;

extern const char *STATE_NAMES[];

/* UART parameters */
typedef struct usart_params1
{
    unsigned int  baud_rate;
    unsigned char data_bit;
    unsigned char stop_bit;
    unsigned char parity;
    unsigned char mark;
    unsigned char space;
    unsigned char usart_mcr_dtr;
    unsigned char usart_mcr_rts;
    unsigned char usart_crtscts;
    unsigned char IX_on;
    unsigned char IX_off; //XonXoff
} usart_params1_t;

/* LED status tracking */
typedef struct
{
    uint16_t tx_count;
    uint16_t rx_count;
    uint16_t sample_tick_cnt_tx;
    uint16_t sample_tick_cnt_rx;
    uint16_t sample_period_ticks_tx;
    uint16_t sample_period_ticks_rx;
    uint8_t tx_led_state;   // 0=灭,1=亮
    uint8_t rx_led_state;   // 0=灭,1=亮
} uart_led_stat_t;

/* Main configuration structure */
typedef struct
{
    usart_params1_t config;
    
    MSG_Q_ID msg_queue;
    int server_fdcmd;
    
    /* sock */
    int sock_cmd;
    int sock_data;
    uint16_t sock_cmd_port;
    uint16_t sock_data_port;
    
    /* client fd*/
    int cmd_client_fd;
    int data_client_fd;
    uint64_t cmd_count;
    uint64_t data_count;

    /* cmd state */
    uint8_t sock_cmd_state;
    uint8_t sock_data_state;

    /* buffer */
    char tx_buffer[BUFFERCOM_SIZE_TX];
    char rx_buffer[BUFFERCOM_SIZE_RX];
    ring_buffer_t data_tx;
    ring_buffer_t data_rx;
    
    /* heartbeat */
    unsigned int last_send_tick; 
    BOOL is_active;           
    
    /* tx chunk limit */
    int tx_buffer_limit; 

    ULONG last_activity_time;
} UART_Config_Params;

/* Time tracking helper */
typedef struct
{
    UINT32 startTick;   // 起始 tick
    int    clkRate;     // 系统时钟频率（tick/s）
} TimeCounter;

/* Function declarations shared across modules */
void timeStart(TimeCounter *tc);
void timeEnd(TimeCounter *tc, UINT32 *pTicks, UINT32 *pMs);
int msToTicks(int ms);
int calc_tx_buffer_limit(uint32_t baudrate, int tick_rate);
uint16_t calc_sample_period(uint32_t baudrate, uint32_t sys_tick_hz, int task_delay_tick);

/* Global variables */
extern UART_Config_Params uart_instances[NUM_PORTS];
extern uart_led_stat_t uart_led_stats[NUM_PORTS];
extern const int portdata_array[];
extern const int portcmd_array[];

#define HEARTBEAT_INTERVAL_SEC 5  // 5秒间隔
#define MAX_HEARTBEAT_TASKS 16    // 最大任务数

/* Task priorities */
#define TCP_SERVER_TASK_PRIO        (92)
#define UART_TX_TASK_PRIO           (85)
#define UART_RX_TASK_PRIO           (75)
#define UART_RX_FORWARD_TASK_PRIO   (92)

/* Task stack sizes */
#define TCP_SERVER_TASK_STACK 8192
#define UART_TASK_STACK 8192

/* Task delays */
#define TCP_TASK_MIN_DELAY                  (2)
#define UART_TX_TASK_MIN_DELAY              (2)
#define UART_RX_TASK_MIN_DELAY              (2)
#define UART_RX_FORWARD_TASK_MIN_DELAY      (15)

#define SAMPLE_BYTES                 16  // 采样字节数基准

#define PL_AXI_WriteReg(BaseAddress, RegOffset, Data) \
      sysAxiWriteLong((BaseAddress) + (RegOffset), (uint32_t)(Data))

#define PL_AXI_ReadReg(BaseAddress, RegOffset) \
    sysAxiReadLong((BaseAddress) + (RegOffset))

#define PL_AXI_BASE  0x40000000   

#endif /* COMMON_H_ */
