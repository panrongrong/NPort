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

#define BASE_LINE 100   //֡����ĿǰΪ100�����Ա��
#define BUFF_TEMP 8096  //��������
#define BUFF_IMAGE 16384 //ͼ��֡��������

typedef enum {
	TYPE4 = 3,
	TYPE3 = 2,
	TYPE2 = 1
}GPPACK_LENGTH_TYPE;


//���ȼ���������4>����3>����2
typedef struct _pack_head_t
{
	int pack_length2; //����2
	int pack_length3; //����3
	int pack_length4; //����4
}pack_head_t;
pack_head_t temp_t;



typedef struct 
{
	int socket_s; //socket���
	int deep;  //��Ϣ�������
	int width;  //��Ϣ���п��
	char * IPADDR; //ip��ַ
	int port;  //�˿ں�
	MSG_Q_ID recv_m;  //��Ϣ����
}DM_message_recv;
DM_message_recv dm_recv;



typedef struct
{
	int  type;
	int  length;
}DM_message_width;


typedef struct 
{
	int socket_s; //socket���
	int deep;  //��Ϣ�������
	//int width;  //��Ϣ���п��
	DM_message_width width_s;
	//char * IPADDR; //ip��ַ
	//int port;  //�˿ں�
	MSG_Q_ID send_m;  //��Ϣ����
}DM_message_send;
DM_message_send dm_send;




typedef struct 
{
	int socket_s; //socket���
	int deep;  //��Ϣ�������
	int width;  //��Ϣ���п��
	char * IPADDR; //ip��ַ
	int port;  //�˿ں�
	MSG_Q_ID recv_m;  //��Ϣ����
	int (*jz_recv_data)(int sock,MSG_Q_ID msg); //�ص�����
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
	int socket_s; //socket���
	int deep;  //��Ϣ�������
	int width;  //��Ϣ���п��
	MSG_Q_ID send_m;  //��Ϣ����
}JZ_message_send;
JZ_message_send jz_send;


typedef struct
{
	char *images_total; //ͼ��֡��
	int images_total_length; //ͼ��֡�ܳ���
	char images_nums;//ͼ��֡���
	char images_unpack; //������
	char images_high; //ͼ��֡��λ
	char images_low; //ͼ��֡��λ
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
