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
//#define BASE_LINE 100   //֡����ĿǰΪ100�����Ա��
//#define BUFF_TEMP 8096  //��������
//#define BUFF_IMAGE 16384 //ͼ��֡��������
//
//typedef enum {
//	TYPE4 = 3,
//	TYPE3 = 2,
//	TYPE2 = 1
//}GPPACK_LENGTH_TYPE;
//
//
////���ȼ���������4>����3>����2
//typedef struct _pack_head_t
//{
//	int pack_length2; //����2
//	int pack_length3; //����3
//	int pack_length4; //����4
//}pack_head_t;
//pack_head_t temp_t;
//
//
//
//typedef struct 
//{
//	int socket_s; //socket���
//	int deep;  //��Ϣ�������
//	int width;  //��Ϣ���п��
//	char * IPADDR; //ip��ַ
//	int port;  //�˿ں�
//	MSG_Q_ID recv_m;  //��Ϣ����
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
//	int socket_s; //socket���
//	int deep;  //��Ϣ�������
//	//int width;  //��Ϣ���п��
//	DM_message_width width_s;
//	//char * IPADDR; //ip��ַ
//	//int port;  //�˿ں�
//	MSG_Q_ID send_m;  //��Ϣ����
//}DM_message_send;
//DM_message_send dm_send;
//
//
//
//
//typedef struct 
//{
//	int socket_s; //socket���
//	int deep;  //��Ϣ�������
//	int width;  //��Ϣ���п��
//	char * IPADDR; //ip��ַ
//	int port;  //�˿ں�
//	MSG_Q_ID recv_m;  //��Ϣ����
//	int (*jz_recv_data)(int sock,MSG_Q_ID msg); //�ص�����
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
//	int socket_s; //socket���
//	int deep;  //��Ϣ�������
//	int width;  //��Ϣ���п��
//	MSG_Q_ID send_m;  //��Ϣ����
//}JZ_message_send;
//JZ_message_send jz_send;
//
//
//typedef struct
//{
//	char *images_total; //ͼ��֡��
//	int images_total_length; //ͼ��֡�ܳ���
//	char images_nums;//ͼ��֡���
//	char images_unpack; //������
//	char images_high; //ͼ��֡��λ
//	char images_low; //ͼ��֡��λ
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
    int sock_cmd;               // Socket�ļ�������
    struct sockaddr_in serverAddr_cmd;
    struct sockaddr_in clientAddr_cmd;
    socklen_t clientLen_cmd;
    
    int sock_data;              // Socket�ļ�������
    struct sockaddr_in serverAddr_data;
    struct sockaddr_in clientAddr_data;
    socklen_t clientLen_data;
    
    uint16_t sock_cmd_port;       // Socket�˿ں�
    uint16_t sock_data_port;       // Socket�˿ں�

    char tx_buffer[1024];          // ���ͻ�����ָ��
    char rx_buffer[1024];          // ���ջ�����ָ��

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
