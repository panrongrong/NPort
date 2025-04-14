#include <stdlib.h>
#include <string.h>
#include "wdLib.h"
#include <vxWorks.h>
#include <stdio.h>
#include "semLib.h"
#include <taskLib.h>
#include <spinLockLib.h>
#include <unistd.h>
#include <vxWorks.h>
#include <vxWorksCommon.h>
#include <errnoLib.h>
#include <ioLib.h>
#include <intLib.h>
#include <logLib.h>
#include <vxAtomicLib.h>
#include <sioLibCommon.h>
#include <msgQLib.h>
#include "sockLib.h"
#include <taskLib.h>
#include <unistd.h>
#include <sockLib.h>
#include <netinet/in.h>
#include <inetLib.h>
#include <stdio.h>
#include "Socket.h"
#include "pack.h"

#define JZ_TEST
char ip1[16] = "192.168.2.10";
struct ip_mreq mreq;

#ifdef DM_TEST
/**********************DM**********************/
TASK_ID  DM_recv,	DM_send ,DM_test1; 
static int dm_len=sizeof(struct sockaddr_in);
struct sockaddr_in dm_serAddr, dm_cliAddr;
struct sockaddr_in dm_readAddr;
void net_data_rec_dmtask();
void net_data_send_dmtask(int socks);
int dm_port[5] = {10203, 10502, 10503, 10301, 10302};
int image_g_pack=0; //图像包组包标识
int Create_net_dmread(DM_message_recv *data);
int Create_net_dmsend(DM_message_send *data);
#endif


#ifdef JZ_TEST
/**********************JZ**********************/
struct sockaddr_in jz_serAddr, jz_cliAddr;
struct sockaddr_in jz_readAddr;
static int jz_len=sizeof(struct sockaddr_in);
int jz_port[5] = {10203, 10502, 10503, 10301, 10302};
/*
 * 端口和通道号映射数组
#define NUM_PORTS 16
const int port_array[NUM_PORTS] = {950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960, 961, 962, 963, 964, 965};
const int channel_array[NUM_PORTS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
//十六个消息队列ID
MSG_Q_ID port_meg1;
MSG_Q_ID port_meg2;
MSG_Q_ID port_meg3;
MSG_Q_ID port_meg4;
MSG_Q_ID port_meg5;

MSG_Q_ID port_meg6;
MSG_Q_ID port_meg7;
MSG_Q_ID port_meg8;
MSG_Q_ID port_meg9;
MSG_Q_ID port_meg10;

MSG_Q_ID port_meg11;
MSG_Q_ID port_meg12;
MSG_Q_ID port_meg13;
MSG_Q_ID port_meg14;
MSG_Q_ID port_meg15;
MSG_Q_ID port_meg16;
*/
MSG_Q_ID jz_meg1;
MSG_Q_ID jz_meg2;
MSG_Q_ID jz_meg3;
MSG_Q_ID jz_meg4;
MSG_Q_ID jz_meg5;
MSG_Q_ID send1;
MSG_Q_ID send2;
MSG_Q_ID send3;
TASK_ID  JZ_send;
int net_data_rec_jztask1(int sock,MSG_Q_ID msg);
int net_data_rec_jztask2(int sock,MSG_Q_ID msg);
int net_data_rec_jztask3(int sock,MSG_Q_ID msg);
int net_data_rec_jztask4(int sock,MSG_Q_ID msg);
int net_data_rec_jztask5(int sock,MSG_Q_ID msg);
void net_data_send_jztask(void);
int Create_net_read_jz(JZ_message_recv *data);
int Create_net_send_jz(JZ_message_send *data);
int length=0;
int aa=1;
char images_nums=0;
char images_unpacknums=1;
int image_flag=0;



MSG_Q_ID jz_test1;
MSG_Q_ID jz_test2;
MSG_Q_ID jz_test3;
MSG_Q_ID jz_test4;
MSG_Q_ID jz_test5;
extern MSG_Q_ID NET_Rec_MSG; 
extern MSG_Q_ID NET_Rec_MSG1; 
extern MSG_Q_ID NET_Rec_MSG2; 
extern MSG_Q_ID NET_Rec_MSG_Image1;
extern MSG_Q_ID NET_Rec_MSG_Image2;
char temp_buf_im[16384] = {0};
char temp_buf_im1[16384] = {0};
#endif

#ifdef DM_TEST
/**************   DM DM DM DM  ***************************************************/
void init_DM_data()
{
	memset((void *)&dm_recv, 0, sizeof(DM_message_recv));
	memset((void *)&dm_send, 0, sizeof(DM_message_send));
	dm_recv.IPADDR="225.0.0.201";
	dm_recv.deep= 10;
	dm_recv.width = 1400;
	dm_recv.port=10102;
	Create_net_dmread(&dm_recv);
	
	dm_send.deep= 10;
	//dm_send.width = 1400;
	dm_send.width_s.length =1400;
	Create_net_dmsend(&dm_send);
}




int Create_net_dmread(DM_message_recv *data)
{
	/*{
		NET_Rec_MSG
		宽度
		深度
		sockFd
		"226.0.0.22"
		PORT
	}*/
	data->recv_m = msgQCreate(data->deep,data->width,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == data->recv_m){
		logMsg("NET_Rec_MSG failed\n",0,0,0,0,0,0);
	}
		
	data->socket_s=socket(AF_INET,SOCK_DGRAM,0);
	if (data->socket_s == ERROR){
		printf ("create socket error: %s",strerror(errno));
		return ERROR;
	}

	int flag = 1,ret;
	ret = setsockopt(data->socket_s,SOL_SOCKET,SO_BROADCAST,(char *)&flag,sizeof(flag));
	if(ret!=0)
	{
		logMsg("setsockopt failed\n",0,0,0,0,0,0);
		close(data->socket_s);
		return FALSE;
	}
	
	/******************/
	DM_recv= taskSpawn 	   ("dm_recv",
							110,
							VX_SUPERVISOR_MODE,
							0x1000000,
							(FUNCPTR) net_data_rec_dmtask,
							0,0,0,0,0,0,0,0,0,0);	
	return OK;
}


int Create_net_dmsend(DM_message_send *data)
{
	/*{
		sockFd
		NET_Send_MSG
		宽度{1:类型，内容}
		深度
		
	}*/
	
	data->send_m = msgQCreate(data->deep,data->width_s.length,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == data->send_m){
		logMsg("NET_Send_MSG failed\n",0,0,0,0,0,0);
	}
	
	DM_send 	= taskSpawn ("Send_task",
							112,
							VX_SUPERVISOR_MODE,
							0x100000,
							(FUNCPTR) net_data_send_dmtask,
							dm_recv.socket_s,0,0,0,0,0,0,0,0,0);
	return 1;
}

//网口收数据
void net_data_rec_dmtask()
{
	/*for(;;){
		//阻塞读udp
		//发到NET_Rec_MSG消息队列
		
	}*/
	int recv_len = 0;
	char buf[BUFF_TEMP] = {0};
	while(1)
	{
		recv_len = recvfrom(dm_recv.socket_s, buf, sizeof(buf),0, (struct sockaddr*)&dm_readAddr, (socklen_t *)&dm_len);
		if(ERROR == recv_len)
		{
			continue;
		}
		msgQSend(dm_recv.recv_m,buf,recv_len,NO_WAIT,MSG_PRI_NORMAL);
	}
	
}

int get_shangxing170(char *buf,int size)
{
	//非阻塞读NET_Rec_MSG
	//读成功就返回 读取到的数据长度
	//失败返回-1
	int length=0;
	char buff[BUFF_TEMP] = {0};
	length = msgQReceive(dm_recv.recv_m, buff, BUFF_TEMP, NO_WAIT);
	if(length == ERROR)
	{
		return -1;
	}
	size = length;
	memcpy(buf,buff,length);
	return length;
}



int test_counts=0;
//网口发数据
void net_data_send_dmtask(int socks)
{
	/*for(;;){
		//读NET_Send_MSG
		//解类型
		//switch 选择设置组播地址 port
		//网口发送
		
	}*/
	int i=0;
	int length=0;
	char buff[BUFF_TEMP] = {0};
	int type=0;
	struct sockaddr_in *addr_in[5];
	memset(addr_in, 0, sizeof(struct sockaddr_in));
	for (i = 0; i < 5; i++)
	{
		set_net_para(&addr_in[i], SERVER_ADDR, dm_port[i]);
	}
	while(1)
	{
		length = msgQReceive(dm_send.send_m, buff, BUFF_TEMP, WAIT_FOREVER);
		if(length == ERROR)
		{
			printf ("msgQReceive error: %s",strerror(errno));
			continue;
		}
		test_counts++;
		if(test_counts == 6)
		{
			test_counts=0;
		}
		type = dm_send.width_s.type;
		printf("this data form types:%d,lengths:%d",type,length);
		//判断port口
		/*switch(type)
		{
			case 1:sendto(socks, buff, length, 0,  (struct sockaddr *)&addr_in[0], sizeof(struct sockaddr_in));break;
			case 2:sendto(socks, buff, length, 0,  (struct sockaddr *)&addr_in[1], sizeof(struct sockaddr_in));break;
			case 3:sendto(socks, buff, length, 0,  (struct sockaddr *)&addr_in[2], sizeof(struct sockaddr_in));break;
			case 4:sendto(socks, buff, length, 0,  (struct sockaddr *)&addr_in[3], sizeof(struct sockaddr_in));break;
			case 5:sendto(socks, buff, length, 0,  (struct sockaddr *)&addr_in[4], sizeof(struct sockaddr_in));break;
				default:break;
		}*/
	}
	
	
}



int kongkou_yaoce(char *buf, int size)
{
	//解前三字节
	//读100的数据
	//标记消息类型
	//发到NET_Send_MSG消息队列
	int i=0;
	int j=0;
	int k=0;
	char temp_data=0;
	char temp_data1=0;
	char temp_data2=0;
	if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0)
	{
		return -1;
	}	
	temp_data=buf[0]&0x1f;
	if(temp_data>0)
	{
		for(i=0;i<temp_data;i++)
		{
			dm_send.width_s.type = 1;
			
			msgQSend(dm_send.send_m,&buf[3+i*100],100,NO_WAIT,MSG_PRI_NORMAL);//dm_send.send_m
		}
		if(buf[0]>=6)
		{
			return 1;
		}
		temp_data1 =buf[1]&0x1f;
		if(temp_data1>0)
		{
			for(j=i;j<(temp_data+temp_data1);j++)
			{
				dm_send.width_s.type = 2;
				msgQSend(dm_send.send_m,&buf[3+j*100],100,NO_WAIT,MSG_PRI_NORMAL);//dm_send.send_m
			}	
			if(buf[1]==6)
			{
				return 1;
			}
			temp_data2 =buf[2]&0x1f;
			if(temp_data2>0)
			{
				for(k=j;k<(temp_data+temp_data1+temp_data2);k++)
				{
					dm_send.width_s.type = 3;
					msgQSend(dm_send.send_m,&buf[3+k*100],100,NO_WAIT,MSG_PRI_NORMAL);//dm_send.send_m
				}	
				if(buf[2]==6)
				{
					return 1;
				}
			}
		}
	}
	else
	{
		temp_data1=buf[1]&0x1f;
		if(temp_data1>0)
		{
			for(i=0;i<temp_data1;i++)
			{
				dm_send.width_s.type = 2;
				msgQSend(dm_send.send_m,&buf[3+i*100],100,NO_WAIT,MSG_PRI_NORMAL);//dm_send.send_m
			}	
			if(buf[1]>=6)
			{
				return 1;
			}
			temp_data2=buf[2]&0x1f;
			if(temp_data2>0)
			{
				for(j=i;j<(temp_data1+temp_data2);j++)
				{
					dm_send.width_s.type = 3;
					msgQSend(dm_send.send_m,&buf[3+j*100],100,NO_WAIT,MSG_PRI_NORMAL);//dm_send.send_m
				}	
			}
		}
		else
		{
			temp_data2=buf[2]&0x1f;
			if(temp_data2>0)
			{
				for(i=0;i<temp_data2;i++)
				{
					dm_send.width_s.type = 3;
					msgQSend(dm_send.send_m,&buf[3+i*100],100,NO_WAIT,MSG_PRI_NORMAL);//
				}	
			}
		}
	}
	return 1;
}

int org_image_counts =0 ; //图像帧组包时，图像帧编号
int org_image_unpack =1 ; //图像帧组包时，拆包编号
char temp_buff_s[BUFF_IMAGE]={0};
int kongkou_image(char *buf, int size)
{
	//解帧头
	//组包
	//标记类型
	//发到NET_Send_MSG消息队列
	//char temp_buff[BUFF_IMAGE];
	//org_image_counts = buf[0];
	//org_image_unpack = buf[1];
	if(org_image_counts == 255)
	{
		org_image_counts =0;
	}
	if((buf[2]&0x1000) ==0)
	{
		if(buf[1] == 0)
		{
			int temp_length_send=0;
			temp_length_send = (buf[2]<<8) | (buf[3]&0xff);
			memcpy(&temp_buff_s[size*image_g_pack],&buf[4],temp_length_send);
			org_image_unpack =1;
			org_image_counts ++;
			dm_send.width_s.type = 4;
			msgQSend(NET_Send_MSG,temp_buff_s,size*image_g_pack+temp_length_send,NO_WAIT,MSG_PRI_NORMAL);//dm_send.send_m
			image_g_pack = 0;//图像包组包标识
			memset(temp_buff_s,0,BUFF_IMAGE);
		}
		else
		{	
			if(image_g_pack > 0)
			{
				if(org_image_counts != buf[0])  //判断如果上一包帧编号如果不等于现在帧编号 直接清0
				{
					memset(temp_buff_s,0,BUFF_IMAGE);
					return -1;
				}
				if(org_image_unpack + 1 >255)// 判断拆包编号是否大于255，如果大于
				{
					if(buf[1] !=0) //如果目前这一包不等于0
					{
						memset(&temp_buff_s[3+size*image_g_pack],0,size); //直接给数组赋0
					}
					else
					{
						memcpy(temp_buff_s,&buf[3+size*image_g_pack],size);
					}
					
				}
				else
				{
					/*if(org_image_unpack+1 != buf[1])  //如果拆包编号等于目前编号，代表正确
					{
						memset(&temp_buff_s[3+size*image_g_pack],0,size); 
					}*/
				}
				org_image_unpack ++;
			}
			else
			{
				org_image_counts = buf[0];  //图像帧组包时，图像帧编号
				org_image_unpack = buf[1];//图像帧组包时，拆包编号
				if(org_image_unpack !=1)  //判断是否为拆包编号的第一包 如果不是直接抛掉整组数据
				{
					return -1;
				}
			}
			memcpy(&temp_buff_s[size*image_g_pack],&buf[4],size);
			image_g_pack++;//图像包组包标识
		}
	}
	if((buf[2]&0x1000) == 1)
	{
		if(buf[1] == 0)
		{
			image_g_pack = 0;//图像包组包标识
			org_image_unpack =1;
			org_image_counts ++;
			dm_send.width_s.type = 5;
			msgQSend(NET_Send_MSG,temp_buff_s,size*image_g_pack,NO_WAIT,MSG_PRI_NORMAL);//dm_send.send_m
		}
		else
		{	
			if(image_g_pack > 1)
			{
				if(org_image_counts != buf[0])  //判断如果上一包帧编号如果不等于现在帧编号 直接清0
				{
					memset(temp_buff_s,0,BUFF_IMAGE);
					return -1;
				}
				if(org_image_unpack + 1 >255)// 判断拆包编号是否大于255，如果大于
				{
					if(buf[1] !=0) //如果目前这一包不等于0
					{
						memset(&temp_buff_s[3+size*image_g_pack],0,size); //直接给数组赋0
					}
					else
					{
						memcpy(temp_buff_s,&buf[3+size*image_g_pack],size);
					}
					
				}
				else
				{
					if(org_image_unpack + 1 != buf[1])  //如果拆包编号等于目前编号，代表正确
					{
						memset(&temp_buff_s[3+size*image_g_pack],0,size);
					}

				}
				org_image_unpack ++;
			}
			else
			{
				org_image_counts = buf[0];  //图像帧组包时，图像帧编号
				org_image_unpack = buf[1];//图像帧组包时，拆包编号
				if(org_image_unpack !=1)  //判断是否为拆包编号的第一包 如果不是直接抛掉整组数据
				{
					return -1;
				}
			}
			memcpy(temp_buff_s,&buf[3+size*image_g_pack],size);
			image_g_pack++;//图像包组包标识
	}
	
	}
	return size;
}
#endif



#ifdef JZ_TEST
/**************   JZ  ************************/

int init_JZ_data(void)
{
	memset((void *)&jz_recv1, 0, sizeof(JZ_message_recv));
	memset((void *)&jz_recv2, 0, sizeof(JZ_message_recv));
	memset((void *)&jz_recv3, 0, sizeof(JZ_message_recv));
	memset((void *)&jz_recv4, 0, sizeof(JZ_message_recv));
	memset((void *)&jz_recv5, 0, sizeof(JZ_message_recv));
	//类型2结构体初始化
	jz_recv1.IPADDR= "225.0.0.202";
	jz_recv1.deep=10;
	jz_recv1.width=400;
	jz_recv1.port=10203;
	jz_recv1.jz_recv_data= net_data_rec_jztask1;
	jz_recv1.jz_task_name ="jz_recv1";
	//类型3结构体初始化
	jz_recv2.IPADDR= "225.0.0.205";
	jz_recv2.deep=10;
	jz_recv2.width=400;
	jz_recv2.port=10502;
	jz_recv2.jz_recv_data= net_data_rec_jztask2;
	jz_recv2.jz_task_name ="jz_recv2";
	//类型4结构体初始化
	jz_recv3.IPADDR= "225.0.0.205";
	jz_recv3.deep=100;
	jz_recv3.width=15000;
	jz_recv3.port=10503;
	jz_recv3.jz_recv_data= net_data_rec_jztask3;
	jz_recv3.jz_task_name ="jz_recv3";
	//图像包0结构体初始化
	jz_recv4.IPADDR= "225.0.0.203";
	jz_recv4.deep=100;
	jz_recv4.width=16384;
	jz_recv4.port=10301;
	jz_recv4.jz_recv_data= net_data_rec_jztask4;
	jz_recv4.jz_task_name ="jz_recv4";
	//图像包1结构体初始化
	jz_recv5.IPADDR= "225.0.0.203";
	jz_recv5.deep=100;
	jz_recv5.width=16384;
	jz_recv5.port=10302;
	jz_recv5.jz_recv_data= net_data_rec_jztask5;
	jz_recv5.jz_task_name ="jz_recv5";
	//初始化结构体
	Create_net_read_jz(&jz_recv1);
	Create_net_read_jz(&jz_recv2);
	Create_net_read_jz(&jz_recv3);
	Create_net_read_jz(&jz_recv4);
	Create_net_read_jz(&jz_recv5);
	
	jz_send.deep=100;
	jz_send.width=16384;

	//初始化结构体
	Create_net_send_jz(&jz_send);
	
	jz_test1 = msgQCreate(10,16384,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == jz_test1){
		logMsg("NET_Rec_MSG failed\n",0,0,0,0,0,0);
	}
	jz_test2 = msgQCreate(10,16384,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == jz_test2){
		logMsg("NET_Rec_MSG failed\n",0,0,0,0,0,0);
	}
	jz_test3 = msgQCreate(10,16384,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == jz_test3){
		logMsg("NET_Rec_MSG failed\n",0,0,0,0,0,0);
	}
	
	jz_test4 = msgQCreate(10,16384,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == jz_test4){
		logMsg("NET_Rec_MSG failed\n",0,0,0,0,0,0);
	}
	
	jz_test5 = msgQCreate(10,16384,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == jz_test5){
		logMsg("NET_Rec_MSG failed\n",0,0,0,0,0,0);
	}
	
	return 1;
}

int Create_net_read_jz(JZ_message_recv *data)
{
	/*{
		NET_Rec_MSG
		宽度
		深度
		sockFd
		"226.0.0.22"
		PORT
		接收回调函数 net_data_rec_task
	}*/
	
	data->recv_m = msgQCreate(data->deep,data->width,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == data->recv_m){
		logMsg("NET_Rec_MSG failed\n",0,0,0,0,0,0);
	}
	
	
	
	data->socket_s =socket(AF_INET,SOCK_DGRAM,0);
	if (data->socket_s == ERROR){
		printf ("create socket error: %s",strerror(errno));
		return ERROR;
	}

	int flag = 1,ret;
	ret = setsockopt(data->socket_s,SOL_SOCKET,SO_BROADCAST,(char *)&flag,sizeof(flag));
	if(ret!=0)
	{
		logMsg("setsockopt failed\n",0,0,0,0,0,0);
		close(data->socket_s);
		return FALSE;
	}
	
	/* 加入到组播*/  
	bzero((char *) &(mreq), sizeof(mreq));  
	mreq.imr_multiaddr.s_addr = inet_addr(data->IPADDR);    /* 设置组播地址 */
    mreq.imr_interface.s_addr = inet_addr(ip1);          /* 设置本地地址 */
//	mreq.imr_interface.s_addr = htonl(INADDR_ANY);      //inet_addr(ip);            /* 设置本地地址 */
//    mreq.imr_ifindex = ifNameToIfIndex("gem1");       /* 网卡名 */   
	if (setsockopt(data->socket_s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == ERROR) {
		printf("setsockopt %s error %d \n", ip1, errnoGet()); 
		close(data->socket_s); 
		return ERROR;  
	}
	/****************************************************************
	 * 
	 */
	bzero((char *) &(jz_serAddr), jz_len);
	jz_serAddr.sin_family=AF_INET;
	jz_serAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	jz_serAddr.sin_port = htons(data->port);
	memset(jz_serAddr.sin_zero, 0, 8);
	/****************************************************************/
	if (bind(data->socket_s, (struct sockaddr*)&jz_serAddr, jz_len)==OK){
		printf("bind----Connected!\n");
		printf("Addr:%s,Port:%d\n",inet_ntoa(jz_serAddr.sin_addr),ntohs(jz_serAddr.sin_port));
	}
	else{
		printf ("bind socket error: %s\n",strerror(errno));
		return ERROR;
	}
	
	data->jz_task= taskSpawn 	   (data->jz_task_name,
									110,
									VX_SUPERVISOR_MODE,
									0x100000,
									(FUNCPTR) data->jz_recv_data,
									data->socket_s,data->recv_m,0,0,0,0,0,0,0,0);	
	return OK;
}


int Create_net_send_jz(JZ_message_send *data)
{
	/*{
		sockFd
		NET_Send_MSG
		宽度{1:类型，内容}
		深度
		
	}*/
	
	data->send_m = msgQCreate(data->deep,data->width,MSG_Q_FIFO );
	if(MSG_Q_ID_NULL  == data->send_m){
		logMsg("NET_Send_MSG failed\n",0,0,0,0,0,0);
	}
	
	JZ_send 	= taskSpawn ("jz_send",
							112,
							VX_SUPERVISOR_MODE,
							0x100000,
							(FUNCPTR) net_data_send_jztask,
							0,0,0,0,0,0,0,0,0,0);
	return 1;
}

//网口收数据
int net_data_rec_jztask1(int sock,MSG_Q_ID msg)
{
		//阻塞读udp
		//发到NET_Rec_MSG消息队列
	int recv_len = 0;
	char buf[BUFF_TEMP] = {0};
	while(1)
	{
		recv_len = recvfrom(sock, buf, sizeof(buf),0, (struct sockaddr*)&jz_readAddr, (socklen_t *)&jz_len);
		if(ERROR == recv_len)
		{
			continue;
		}
		msgQSend(msg,buf,recv_len,NO_WAIT,MSG_PRI_NORMAL);
	}
		
	
}
int net_data_rec_jztask2(int sock,MSG_Q_ID msg)
{
	
}

int net_data_rec_jztask3(int sock,MSG_Q_ID msg)
{
	
}

int net_data_rec_jztask4(int sock,MSG_Q_ID msg)
{
	
}

int net_data_rec_jztask5(int sock,MSG_Q_ID msg)
{
	
}

int get_xiaxing700(UINT8 *buf,int size)
{
	//非阻塞读NET_Rec_MSG
	//做优先级组包
	//读成功就返回 读取到的数据长度
	//失败返回-1
	char i=0;
	char j=0;
	char k=0;
	int length1=0;
	int length2=0;
	int length3=0;
	char temp_buf1[BUFF_TEMP] = {0};
	char temp_buf2[BUFF_TEMP] = {0};
	char temp_buf3[BUFF_TEMP] = {0};
	for(i=0;i<size/BASE_LINE;i++)
	{
		length1 = msgQReceive(NET_Rec_MSG, temp_buf1, BUFF_TEMP, NO_WAIT);//4
		if(length1 ==ERROR )
		{
		   break;
		}
		memcpy(&buf[3+i * BASE_LINE],temp_buf1,BASE_LINE);
	}
	buf[0] = 1<<7 | i;
	if(i==6)
	{
		return size;
	}
	for(j=i;j<size/BASE_LINE;j++)
	{
		length2 = msgQReceive(NET_Rec_MSG1, temp_buf2, BUFF_TEMP, NO_WAIT);//3
		if(length2 ==ERROR )
		{
			break;
		}
		memcpy(&buf[3+j * BASE_LINE],temp_buf2,BASE_LINE);		
	}
	buf[1] = 1<<6 | j;
	if(j==6)
	{
		return size;
	}
	for(k=j;k<size/BASE_LINE;k++)
	{
		length3 = msgQReceive(NET_Rec_MSG2, temp_buf3, BUFF_TEMP, NO_WAIT);//2
		if(length3 ==ERROR )
		{
			break;
		}
		memcpy(&buf[3+ k * BASE_LINE],temp_buf3,BASE_LINE);	
	}
	buf[2] = 1<<5 | k;
	return length1;
}


int get_image(UINT8 *buf,int size)
{
	//判断上一次的数据发完没
	//非阻塞读NET_Rec_MSG
	//读成功拆帧  失败返回-1
	//
	//char temp_buf[16384] = {0};
	if(aa==1)
	{
		length = msgQReceive(NET_Rec_MSG_Image1, temp_buf_im, 16384, NO_WAIT);//4	
		if(length ==ERROR)
		{
			return -1;
		}
		if(length < size)
		{
			aa     = 1;
			buf[0] = images_nums;
			buf[1] = 0;
			buf[2] = (length & 0x0f) | (0<<4);
			buf[3] =  length & 0xff;
			memcpy(&buf[4],&temp_buf_im[image_flag*size],size);
			images_nums ++;
			if(images_nums == 255)
			{
				images_nums = 0;
			}
			images_unpacknums = 1;
			return length;
		}
		else
		{
			aa =0;
			buf[0] = images_nums;
			buf[1] = images_unpacknums;
			buf[2] = (size & 0x0f) | (0<<4);
			buf[3] =  size & 0xff;
			memcpy(&buf[4],&temp_buf_im[image_flag*size],size);
			image_flag ++;
			images_unpacknums++;
			length = length -size;
			return size;
		}
	}
	else
	{
		if(length >size)
		{
			buf[0] = images_nums;
			buf[1] = images_unpacknums;
			buf[2] = (size & 0x0f) | (0<<4);
			buf[3] =  size & 0xff;
			memcpy(&buf[4],&temp_buf_im[image_flag*size],size);
			image_flag ++;
			images_unpacknums++;
			length = length -size;
			if(images_nums == 255)
			{
				images_nums = 0;
			}	
			if(images_unpacknums == 255)
			{
				images_unpacknums = 1;
			}
			return size;
		}
		else
		{ 	
			aa     = 1;
			buf[0] = images_nums;
			buf[1] = 0;
			buf[2] = (length & 0x0f) | (0<<4);
			buf[3] =  length & 0xff;
			memcpy(&buf[4],&temp_buf_im[image_flag*size],size);
			images_nums ++;
			if(images_nums == 255)
			{
				images_nums = 0;
			}
			images_unpacknums = 1;
			return length;
		}
	}
}



int get_image1(UINT8 *buf,int size)
{
	//判断上一次的数据发完没
	//非阻塞读NET_Rec_MSG
	//读成功拆帧  失败返回-1
	//
	//char temp_buf[16384] = {0};
	if(aa==1)
	{
		length = msgQReceive(NET_Rec_MSG_Image2, temp_buf_im1, 16384, NO_WAIT);//4	
		if(length ==ERROR)
		{
			return -1;
		}
		if(length < size)
		{
			aa     = 1;
			buf[0] = images_nums;
			buf[1] = 0;
			buf[2] = (length & 0x0f) | (1<<4);
			buf[3] =  length & 0xff;
			memcpy(&buf[4],&temp_buf_im1[image_flag*size],size);
			images_nums ++;
			if(images_nums == 255)
			{
				images_nums = 0;
			}
			images_unpacknums = 1;
			return length;
		}
		else
		{
			aa =0;
			buf[0] = images_nums;
			buf[1] = images_unpacknums;
			buf[2] = (size & 0x0f) | (1<<4);
			buf[3] =  size & 0xff;
			memcpy(&buf[4],&temp_buf_im1[image_flag*size],size);
			image_flag ++;
			images_unpacknums++;
			length = length -size;
			return size;
		}
	}
	else
	{
		if(length >size)
		{
			buf[0] = images_nums;
			buf[1] = images_unpacknums;
			buf[2] = (size & 0x0f) | (1<<4);
			buf[3] =  size & 0xff;
			memcpy(&buf[4],&temp_buf_im1[image_flag*size],size);
			image_flag ++;
			images_unpacknums++;
			length = length -size;
			if(images_nums == 255)
			{
				images_nums = 0;
			}	
			if(images_unpacknums == 255)
			{
				images_unpacknums = 1;
			}
			return size;
		}
		else
		{ 	
			aa     = 1;
			buf[0] = images_nums;
			buf[1] = 0;
			buf[2] = (length & 0x0f) | (1<<4);
			buf[3] =  length & 0xff;
			memcpy(&buf[4],&temp_buf_im1[image_flag*size],size);
			images_nums ++;
			if(images_nums == 255)
			{
				images_nums = 0;
			}
			images_unpacknums = 1;
			return length;
		}
	}
}

//网口发数据
void net_data_send_jztask(void)
{
	/*for(;;){
		//读NET_Send_MSG
		//解类型
		//switch 选择设置组播地址 port
		//网口发送
		
	}*/
	int i=0;
	int length=0;
	char buff[BUFF_TEMP] = {0};
	int type=0;
	struct sockaddr_in *addr_in[5];
	memset(addr_in, 0, sizeof(struct sockaddr_in));
	for (i = 0; i < 5; i++)
	{
		set_net_para(&addr_in[i], SERVER_ADDR, jz_port[i]);
	}
	while(1)
	{
		length = msgQReceive(dm_send.send_m, buff, BUFF_TEMP, WAIT_FOREVER);
		if(length == ERROR)
		{
			printf ("msgQReceive error: %s",strerror(errno));
			continue;
		}
		
		//判断port口
		switch(type)
		{
			case 1:sendto(dm_send.socket_s, buff, length, 0,  (struct sockaddr *)&addr_in[0], sizeof(struct sockaddr_in));break;
			case 2:sendto(dm_send.socket_s, buff, length, 0,  (struct sockaddr *)&addr_in[1], sizeof(struct sockaddr_in));break;
			case 3:sendto(dm_send.socket_s, buff, length, 0,  (struct sockaddr *)&addr_in[2], sizeof(struct sockaddr_in));break;
			case 4:sendto(dm_send.socket_s, buff, length, 0,  (struct sockaddr *)&addr_in[3], sizeof(struct sockaddr_in));break;
			case 5:sendto(dm_send.socket_s, buff, length, 0,  (struct sockaddr *)&addr_in[4], sizeof(struct sockaddr_in));break;
				default:break;
		}
	}
	
	
	
}


//Y->eth
int kongkou_yaokong170(char *buf, int size)//700
{
	//读取前170
	//标记类型
	//发到NET_Send_MSG消息队列
	
	if(size  ==700)
	{
		msgQSend(dm_send.send_m,buf,size,NO_WAIT,MSG_PRI_NORMAL);
	}
	if(size == 170)
	{
		msgQSend(dm_send.send_m,buf,size,NO_WAIT,MSG_PRI_NORMAL);
	}
	return size;
}






/**********************test****/
int get_xiaxing700_test(char *buf,int size)
{
	//非阻塞读NET_Rec_MSG
	//做优先级组包
	//读成功就返回 读取到的数据长度
	//失败返回-1
	char i=0;
	char j=0;
	char k=0;
	int length1=0;
	int length2=0;
	int length3=0;
	char temp_buf1[BUFF_TEMP] = {0};
	char temp_buf2[BUFF_TEMP] = {0};
	char temp_buf3[BUFF_TEMP] = {0};
	for(i=0;i<size/BASE_LINE;i++)
	{
		length1 = msgQReceive(jz_test1, temp_buf1, BUFF_TEMP, NO_WAIT);//4
		if(length1 ==ERROR )
		{
		   break;
		}
		memcpy(&buf[3+i * BASE_LINE],temp_buf1,BASE_LINE);
	}
	buf[0] = 1<<7 | i;
	if(i==6)
	{
		return size;
	}
	for(j=i;j<size/BASE_LINE;j++)
	{
		length2 = msgQReceive(jz_test2, temp_buf2, BUFF_TEMP, NO_WAIT);//3
		if(length2 ==ERROR )
		{
			break;
		}
		memcpy(&buf[3+j * BASE_LINE],temp_buf2,BASE_LINE);		
	}
	buf[1] = 1<<6 | j;
	if(j==6)
	{
		return size;
	}
	for(k=j;k<size/BASE_LINE;k++)
	{
		length3 = msgQReceive(jz_test3, temp_buf3, BUFF_TEMP, NO_WAIT);//2
		if(length3 ==ERROR )
		{
			break;
		}
		memcpy(&buf[3+ k * BASE_LINE],temp_buf3,BASE_LINE);	
	}
	buf[2] = 1<<5 | k;
	return length1;
}



/*int get_image_test(char *buf,unsigned int size)
{
	//判断上一次的数据发完没
	//非阻塞读NET_Rec_MSG
	//读成功拆帧  失败返回-1
	//
	//char temp_buf[16384] = {0};
	if(aa==1)
	{
		length = msgQReceive(jz_test4, temp_buf_im, 16384, NO_WAIT);//4	
		if(length ==ERROR)
		{
			return -1;
		}
		if(length < size)
		{
			aa     = 1;
			buf[0] = images_nums;
			buf[1] = 0;
			buf[2] = length &0xf0;
			buf[3] = length &0x0f;
			memcpy(&buf[4],&temp_buf_im[image_flag*size],size);
			images_nums ++;
			if(images_nums == 255)
			{
				images_nums = 0;
			}
			images_unpacknums = 1;
			return length;
		}
		else
		{
			aa =0;
			buf[0] = images_nums;
			buf[1] = images_unpacknums;
			buf[2] = size &0xf0;
			buf[3] = size &0x0f;
			memcpy(&buf[4],&temp_buf_im[image_flag*size],size);
			image_flag ++;
			images_unpacknums++;
			length = length -size;
			return size;
		}
	}
	else
	{
		if(length >size)
		{
			buf[0] = images_nums;
			buf[1] = images_unpacknums;
			buf[2] = size &0xf0;
			buf[3] = size &0x0f;
			memcpy(&buf[4],&temp_buf_im[image_flag*size],size);
			image_flag ++;
			images_unpacknums++;
			length = length -size;
			if(images_nums == 255)
			{
				images_nums = 0;
			}	
			if(images_unpacknums == 255)
			{
				images_unpacknums = 1;
			}
			return size;
		}
		else
		{ 	
			aa     = 1;
			buf[0] = images_nums;
			buf[1] = 0;
			buf[2] = length &0xf0;
			buf[3] = length &0x0f;
			memcpy(&buf[4],&temp_buf_im[image_flag*size],size);
			images_nums ++;
			if(images_nums == 255)
			{
				images_nums = 0;
			}
			images_unpacknums = 1;
			return length;
		}
	}
}*/
#endif



