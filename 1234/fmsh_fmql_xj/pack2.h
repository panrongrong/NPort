#ifndef PACK_H_
#define PACK_H_
#include <netinet/in.h>

#include "ringbuffer.h"

#define BUFFERCOM_SIZE 4096
#define NUM_PORTS       16

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
}usart_params1_t;

typedef struct {

    usart_params1_t config; 
    /* */ 
    MSG_Q_ID msg_queue;          
    int server_fdcmd;	
    /* uart base */

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
    char tx_buffer[BUFFERCOM_SIZE];       
    char rx_buffer[BUFFERCOM_SIZE];    
    ring_buffer_t data_tx;
    ring_buffer_t data_rx;

    
} UART_Config_Params;

#endif
