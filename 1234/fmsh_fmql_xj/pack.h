//#ifndef PACK_H_
//#define PACK_H_
///*上面代码实现连接多端口连接tcp*/
//#define BUFFERCOM_SIZE 1024
//#define     STATE_INIT      0
//#define     STATE_CONN_FAIL 1
//#define     STATE_TCP_OPEN  2
//#define     STATE_TCP_CONN  3
//#define     STATE_TCP_CLOSE 4
//#define     STATE_TCP_WAIT  5
//#define     STATE_RW_DATA   6
//#define NUM_PORTS 16
//
//const char* STATE_NAMES[] = {
//    "STATE_INIT",       
//    "STATE_CONN_FAIL",  
//    "STATE_TCP_OPEN",   
//    "STATE_TCP_CONN",   
//    "STATE_TCP_CLOSE",  
//    "STATE_TCP_WAIT",   
//    "STATE_RW_DATA",    
//};
//const int portdata_array[] = {950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960, 961, 962, 963, 964, 965};
//const int channel_array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
//const int portcmd_array[] = {966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981};
//int bauderate_table[] = { 300, 600, 1200, 2400, 4800, 7200, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 150, 134, 110, 75, 50};
//unsigned int data_bit_table[] = { 5, 6, 7, 8 };
//
//typedef struct usart_params1
//{
//	unsigned int  baud_rate;
//	unsigned char data_bit;
//	unsigned char stop_bit;
//	unsigned char parity;
//	unsigned char mark;
//	unsigned char space;
//	unsigned char usart_mcr_dtr;
//	unsigned char usart_mcr_rts;
//	unsigned char usart_crtscts;
//	unsigned char IX_on;
//	unsigned char IX_off; //XonXoff
//}usart_params1_t;
//
//typedef struct {
//    usart_params1_t config;  
//    MSG_Q_ID msg_queue;          
//
//    int server_fdcmd;	
//    int sock_cmd;               // Socket文件描述符
//    struct sockaddr_in serverAddr_cmd;
//    struct sockaddr_in clientAddr_cmd;
//    socklen_t clientLen_cmd;
//    
//    int sock_data;              // Socket文件描述符
//    struct sockaddr_in serverAddr_data;
//    struct sockaddr_in clientAddr_data;
//    socklen_t clientLen_data;
//    
//    uint16_t sock_cmd_port;       // Socket端口号
//    uint16_t sock_data_port;       // Socket端口号
//
//    char tx_buffer[1024];          // 发送缓冲区指针
//    char rx_buffer[1024];          // 接收缓冲区指针
//
//    uint8_t state; 
//} UART_Config_Params;
//UART_Config_Params uart_instances[NUM_PORTS];
//
//void creat_tcp(UART_Config_Params *uart_instance, int port);
//void bind_tcp(UART_Config_Params *uart_instance, int port);
//void listen_tcp(UART_Config_Params *uart_instance, int port);
//void accept_tcp(UART_Config_Params *uart_instance, int port);
//void recv_uartdata(UART_Config_Params *uart_instance);
//void processUartInstances(UART_Config_Params *uart_instances, int num_ports);
//
//#endif
#ifndef PACK_H_
#define PACK_H_

#define BUFFERCOM_SIZE 1024
#define     STATE_INIT      0
#define     STATE_CONN_FAIL 1
#define     STATE_TCP_OPEN  2
#define     STATE_TCP_CONN  3
#define     STATE_TCP_CLOSE 4
#define     STATE_TCP_WAIT  5
#define     STATE_RW_DATA   6
#define NUM_PORTS 16

const char* STATE_NAMES[] = {
    "STATE_INIT",       
    "STATE_CONN_FAIL",  
    "STATE_TCP_OPEN",   
    "STATE_TCP_CONN",   
    "STATE_TCP_CLOSE",  
    "STATE_TCP_WAIT",   
    "STATE_RW_DATA",    
};
const int portdata_array[] = {950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960, 961, 962, 963, 964, 965};
const int channel_array[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
const int portcmd_array[] = {966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981};
int bauderate_table[] = { 300, 600, 1200, 2400, 4800, 7200, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 150, 134, 110, 75, 50};
unsigned int data_bit_table[] = { 5, 6, 7, 8 };

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
    MSG_Q_ID msg_queue;          

    int server_fdcmd;	
    int sock_cmd;               // Socket文件描述符
    struct sockaddr_in serverAddr_cmd;
    struct sockaddr_in clientAddr_cmd;
    socklen_t clientLen_cmd;
    
    int server_fddata;
    int sock_data;              // Socket文件描述符
    struct sockaddr_in serverAddr_data;
    struct sockaddr_in clientAddr_data;
    socklen_t clientLen_data;
    
    uint16_t sock_cmd_port;       // Socket端口号
    uint16_t sock_data_port;       // Socket端口号
    
    int choose_channel;

    char tx_buffer[1024];          // 发送缓冲区指针
    char rx_buffer[1024];          // 接收缓冲区指针

    uint8_t state; 
} UART_Config_Params;
UART_Config_Params uart_instances[NUM_PORTS];

void creat_tcp(UART_Config_Params *uart_instance);
void bind_tcp(UART_Config_Params *uart_instance);
void listen_tcp(UART_Config_Params *uart_instance);
void accept_tcp(UART_Config_Params *uart_instance);
void creat_tcpdata(UART_Config_Params *uart_instance);
void bind_tcpdata(UART_Config_Params *uart_instance);
void listen_tcpdata(UART_Config_Params *uart_instance);
void accept_tcpdata(UART_Config_Params *uart_instance);
void recv_tcpdata_task(UART_Config_Params *uart_instance);
void recv_uartdata_task(UART_Config_Params *uart_instance);
void processUartInstances(UART_Config_Params *uart_instances, int num_ports);

#endif
