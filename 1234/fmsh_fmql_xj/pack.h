#ifndef PACK_H_
#define PACK_H_

//typedef enum {
//	TYPE_700BYTE = 700,
//	TYPE_1400BYTE = 1400,
//	TYPE_2000BYTE = 2000
//}GPPACK_TYPE;
//
//typedef enum {
//	TYPE_730BYTE_IMAGE  = 730,
//	TYPE_1480BYTE_IMAGE = 1480,
//	TYPE_2000BYTE_IMAGE = 2000
//}UNPACKIMAGE_TYPE;
//
//#define BASE_LINE 100   //帧长度目前为100，可以变更
//#define BUFF_TEMP 8096  //缓存数组
//#define BUFF_IMAGE 16384 //图像帧缓存数组
//
//typedef enum {
//	TYPE4 = 3,
//	TYPE3 = 2,
//	TYPE2 = 1
//}GPPACK_LENGTH_TYPE;
//
//
////优先级排序，类型4>类型3>类型2
//typedef struct _pack_head_t
//{
//	int pack_length2; //类型2
//	int pack_length3; //类型3
//	int pack_length4; //类型4
//}pack_head_t;
//pack_head_t temp_t;
//
//
//
//typedef struct 
//{
//	int socket_s; //socket句柄
//	int deep;  //消息队列深度
//	int width;  //消息队列宽度
//	char * IPADDR; //ip地址
//	int port;  //端口号
//	MSG_Q_ID recv_m;  //消息队列
//}DM_message_recv;
//DM_message_recv dm_recv;
//
//
//
//typedef struct
//{
//	int  type;
//	int  length;
//}DM_message_width;
//
//
//typedef struct 
//{
//	int socket_s; //socket句柄
//	int deep;  //消息队列深度
//	//int width;  //消息队列宽度
//	DM_message_width width_s;
//	//char * IPADDR; //ip地址
//	//int port;  //端口号
//	MSG_Q_ID send_m;  //消息队列
//}DM_message_send;
//DM_message_send dm_send;
//
//
//
//
//typedef struct 
//{
//	int socket_s; //socket句柄
//	int deep;  //消息队列深度
//	int width;  //消息队列宽度
//	char * IPADDR; //ip地址
//	int port;  //端口号
//	MSG_Q_ID recv_m;  //消息队列
//	int (*jz_recv_data)(int sock,MSG_Q_ID msg); //回调函数
//	TASK_ID jz_task;    //taskid
//	char * jz_task_name;  //taskname
//}JZ_message_recv;
//JZ_message_recv jz_recv1;
//JZ_message_recv jz_recv2;
//JZ_message_recv jz_recv3;
//JZ_message_recv jz_recv4;
//JZ_message_recv jz_recv5;
//
//typedef struct 
//{
//	int socket_s; //socket句柄
//	int deep;  //消息队列深度
//	int width;  //消息队列宽度
//	MSG_Q_ID send_m;  //消息队列
//}JZ_message_send;
//JZ_message_send jz_send;
//
//
//typedef struct
//{
//	char *images_total; //图像帧包
//	int images_total_length; //图像帧总长度
//	char images_nums;//图像帧编号
//	char images_unpack; //拆包编号
//	char images_high; //图像帧高位
//	char images_low; //图像帧低位
//}images_unpack;
//images_unpack images_nums1;
//images_unpack images_nums2;
//
//
//
//
//
//
//
//extern int kongkou_yaoce(char *buf, int size);
//void init_DM_data();
//int get_image_test(char *buf,unsigned int size);
//int kongkou_image(char *buf, int size);
//extern int get_image(UINT8 *buf,int size);
//extern int get_image1(UINT8 *buf,int size);
//extern int get_xiaxing700(UINT8 *buf,int size);

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
    
    int sock_data;              // Socket文件描述符
    struct sockaddr_in serverAddr_data;
    struct sockaddr_in clientAddr_data;
    socklen_t clientLen_data;
    
    uint16_t sock_cmd_port;       // Socket端口号
    uint16_t sock_data_port;       // Socket端口号

    char tx_buffer[1024];          // 发送缓冲区指针
    char rx_buffer[1024];          // 接收缓冲区指针

    uint8_t state; 
} UART_Config_Params;
UART_Config_Params uart_instances[NUM_PORTS];

void creat_tcp(UART_Config_Params *uart_instance, int port);
void bind_tcp(UART_Config_Params *uart_instance, int port);
void listen_tcp(UART_Config_Params *uart_instance, int port);
void accept_tcp(UART_Config_Params *uart_instance, int port);
void recv_uartdata(UART_Config_Params *uart_instance, int port);
void processUartInstances(UART_Config_Params *uart_instances, int num_ports);

#endif
