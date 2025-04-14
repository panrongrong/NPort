#ifndef PACK_H_
#define PACK_H_

typedef enum {
	TYPE_700BYTE = 700,
	TYPE_1400BYTE = 1400,
	TYPE_2000BYTE = 2000
}GPPACK_TYPE;

typedef enum {
	TYPE_730BYTE_IMAGE  = 730,
	TYPE_1480BYTE_IMAGE = 1480,
	TYPE_2000BYTE_IMAGE = 2000
}UNPACKIMAGE_TYPE;

#define BASE_LINE 100   //帧长度目前为100，可以变更
#define BUFF_TEMP 8096  //缓存数组
#define BUFF_IMAGE 16384 //图像帧缓存数组

typedef enum {
	TYPE4 = 3,
	TYPE3 = 2,
	TYPE2 = 1
}GPPACK_LENGTH_TYPE;


//优先级排序，类型4>类型3>类型2
typedef struct _pack_head_t
{
	int pack_length2; //类型2
	int pack_length3; //类型3
	int pack_length4; //类型4
}pack_head_t;
pack_head_t temp_t;



typedef struct 
{
	int socket_s; //socket句柄
	int deep;  //消息队列深度
	int width;  //消息队列宽度
	char * IPADDR; //ip地址
	int port;  //端口号
	MSG_Q_ID recv_m;  //消息队列
}DM_message_recv;
DM_message_recv dm_recv;



typedef struct
{
	int  type;
	int  length;
}DM_message_width;


typedef struct 
{
	int socket_s; //socket句柄
	int deep;  //消息队列深度
	//int width;  //消息队列宽度
	DM_message_width width_s;
	//char * IPADDR; //ip地址
	//int port;  //端口号
	MSG_Q_ID send_m;  //消息队列
}DM_message_send;
DM_message_send dm_send;




typedef struct 
{
	int socket_s; //socket句柄
	int deep;  //消息队列深度
	int width;  //消息队列宽度
	char * IPADDR; //ip地址
	int port;  //端口号
	MSG_Q_ID recv_m;  //消息队列
	int (*jz_recv_data)(int sock,MSG_Q_ID msg); //回调函数
	TASK_ID jz_task;    //taskid
	char * jz_task_name;  //taskname
}JZ_message_recv;
JZ_message_recv jz_recv1;
JZ_message_recv jz_recv2;
JZ_message_recv jz_recv3;
JZ_message_recv jz_recv4;
JZ_message_recv jz_recv5;

typedef struct 
{
	int socket_s; //socket句柄
	int deep;  //消息队列深度
	int width;  //消息队列宽度
	MSG_Q_ID send_m;  //消息队列
}JZ_message_send;
JZ_message_send jz_send;


typedef struct
{
	char *images_total; //图像帧包
	int images_total_length; //图像帧总长度
	char images_nums;//图像帧编号
	char images_unpack; //拆包编号
	char images_high; //图像帧高位
	char images_low; //图像帧低位
}images_unpack;
images_unpack images_nums1;
images_unpack images_nums2;







extern int kongkou_yaoce(char *buf, int size);
void init_DM_data();
int get_image_test(char *buf,unsigned int size);
int kongkou_image(char *buf, int size);
extern int get_image(UINT8 *buf,int size);
extern int get_image1(UINT8 *buf,int size);
extern int get_xiaxing700(UINT8 *buf,int size);




#endif
