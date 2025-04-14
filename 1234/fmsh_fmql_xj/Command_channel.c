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
#include <tickLib.h>
#include "global_baud.h"

#define PORT 966
#define BUFFERCOM_SIZE 1024
#define MAXCOM_CLIENTSFD 10
#define CHOOSE_CHANNEL 7


#define     ASPP_CMD_PORT_INIT          (0x2c)  //初始化串口
#define     ASPP_CMD_NOTIFY             (0x26)  //向中间件通知设备状态改变
#define     ASPP_CMD_SETBAUD            (0x17)  //设置串口波特率
#define     ASPP_CMD_XONXOFF            (0x18)  //XonXoff
#define     ASPP_CMD_TX_FIFO            (0x30)  //TX_FIFO
#define     ASPP_CMD_LINECTRL           (0x12)  //LINECTRL
#define     ASPP_CMD_SETXON             (0x33)  //SETXON
#define     ASPP_CMD_SETXOFF            (0x34)  //SETXOFF
#define     ASPP_CMD_START_BREAK        (0x21)  //START_BREAK
#define     ASPP_CMD_STOP_BREAK         (0x22)  //STOP_BREAK
#define     ASPP_CMD_POLLING            (0x27 )  //POLLING
#define     ASPP_CMD_ALIVE              (0x28)  //ALIVE
#define     ASPP_CMD_WAIT_OQUEUE        (0x2f)  //WAIT_OQUEUE
#define     ASPP_CMD_FLUSH              (0x14)  //FLUSH 

#define ASPP_NOTIFY_PARITY   0x01
#define ASPP_NOTIFY_FRAMING  0x02
#define ASPP_NOTIFY_HW_OVERRUN  0x04
#define ASPP_NOTIFY_SW_OVERRUN  0x08
#define ASPP_NOTIFY_BREAK   0x10
#define ASPP_NOTIFY_MSR_CHG  0x20

#define UART_MSR_CTS  0x10
#define UART_MSR_DSR  0x20
#define UART_MSR_DCD  0x80

/**************************gloable variable *****************************************/

/************************** usart *****************************************/
#define    USART_STOP_BIT_2   0x02
#define    USART_STOP_BIT_1   0x01

#define    USART_PARITY_NONE  0x00
#define    USART_PARITY_EVEN  0x02
#define    USART_PARITY_ODD   0x01
#define    USART_IOCTL_MARK   0x01
#define    USART_IOCTL_SPACE  0x01

#define    LINE_CONTROL_UART_MCR_DTR 0
#define    LINE_CONTROL_UART_MCR_RTS 1

//const int port_array[NUM_PORTS] = {966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981};
int bauderate_table[] = { 300, 600, 1200, 2400, 4800, 7200, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 150, 134, 110, 75, 50};
unsigned int data_bit_table[] = { 5, 6, 7, 8 };


#define FCR_TRIGGER_LEVEL_1  0x00  //FIFO
#define FCR_TRIGGER_LEVEL_16 0x03  //FIFO
#define FCR_FIFO_ENABLE 0x01  // FIFO 使能位
#define MCR_DTR 0x01          // DTR（Data Terminal Ready）位
#define MCR_RTS 0x02          // RTS（Request To Send）位
volatile int heartbeat_task_id = -1;  // 心跳任务 ID


int taskID = 0;
void uart_task(unsigned int channel);
void send_xon_xoff_char(uint8_t channel, uint8_t is_xon);
void heartbeat_send_task();

void set_nonblocking(int sockfd) {
	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int socket_send_to_middle(int sock_fd, char *buf, int buf_len) 
{
	for(int i=0; i<buf_len; i++)
	{
		//printf("buf[%d]:%x\n", i, buf[i]);
	}

	int ret = send(sock_fd, buf, buf_len, 0);
	if (ret < 0) {
		perror("send");
		return -1;
	}
	return 0;
}

//打包数据
void pack_data(char *buf, char *pack_buf)
{
	pack_buf[0] = buf[0];
}

int  init_usart(int client_socket, char *buf, int buf_len) 
{
	int ret;

	unsigned char stop_bit;
	unsigned char data_bit;
	//    unsigned char parity;
	usart_params_t usart_params;

	char pack_buf[5] = {0};
	//提取串口参数

	//设置串口波特率
	int baud_rate = bauderate_table[(int)buf[2]];
	usart_params.baud_rate = baud_rate;
	global_baud_rate = baud_rate; // 保存全局波特率
	printf("baud_rate: %d\n", usart_params.baud_rate);

	//data bit
	data_bit = ((int)buf[3])& 0x03;
	usart_params.data_bit = data_bit_table[data_bit];
	//printf("data_bit: %d\n", usart_params.data_bit);

	//stop bit
	stop_bit = ((int)buf[3])& 0x04;
	if(stop_bit == 0)
	{
		usart_params.stop_bit = USART_STOP_BIT_1;
		//printf("stop_bit: %d\n", usart_params.stop_bit);
	}
	else
	{
		usart_params.stop_bit = USART_STOP_BIT_2;
		//printf("stop_bit: %d\n", usart_params.stop_bit);
	}

	// 0b111000，用于提取第3 - 5位
	int last_mask = ((int)buf[3]) & 0x38;
	switch (last_mask) {
	case 0x8:
		//printf("Parity: Even\n");
		usart_params.parity = USART_PARITY_EVEN;
		break;
	case 0x10:
		//printf("Parity: Odd\n");
		usart_params.parity = USART_PARITY_ODD;
		break;
	case 0x18:
		//printf("Parity: Mark\n");
		usart_params.mark = USART_IOCTL_MARK;
		usart_params.parity = USART_PARITY_NONE;
		break;
	case 0x20:
		//printf("Parity: Space\n");
		usart_params.space = USART_IOCTL_SPACE;
		break;
	default:
		//printf("Unknown parity configuration\n");
		break;
	}
	//调用AXI_api设置串口相关寄存器
	axi165502CInit(CHOOSE_CHANNEL, usart_params);

	usart_params.usart_mcr_dtr = (int)buf[4];

	usart_params.usart_mcr_rts = (int)buf[5];

	// 获取当前 MCR 寄存器值
	unsigned int mcr_reg = userAxiCfgRead(CHOOSE_CHANNEL, AXI_16550_MCR);

	// 设置 DTR 位
	if (usart_params.usart_mcr_dtr) {
		mcr_reg |= MCR_DTR;
	} else {
		mcr_reg &= ~MCR_DTR;
	}

	// 设置 RTS 位
	if (usart_params.usart_mcr_rts) {
		mcr_reg |= MCR_RTS;
	} else {
		mcr_reg &= ~MCR_RTS;
	}
	// 写入更新后的 MCR 寄存器值
	userAxiCfgWrite(CHOOSE_CHANNEL, AXI_16550_MCR, mcr_reg);

	usart_params.usart_crtscts = (int)buf[6];


//	if(usart_params.IX_on == (int)buf[7])
//	{
//		send_xon_xoff_char(CHOOSE_CHANNEL, 1);
//	}
//	if(usart_params.IX_off == (int)buf[8])
//	{
//		send_xon_xoff_char(CHOOSE_CHANNEL, 0);
//	}

	//打包数据
	pack_buf[0] = buf[0];
	pack_buf[1] = 0x3;
	pack_buf[2] = 0x0;
	pack_buf[3] = 0x0;
	pack_buf[4] = 0x0; 
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, pack_buf, sizeof(pack_buf));
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}

	return 0;

}


int  usart_set_baudrate(int client_socket, char *buf, int buf_len)
{
	int ret;
	unsigned int baud_rate;

	//提取波特率:假设高位在前
	baud_rate = buf[2] << 24 | buf[3] << 16 | buf[4] << 8 | buf[5];
	global_baud_rate = baud_rate;
	//printf("baud_rate: %d\n", baud_rate);

	//调用AXI_api设置串口波特率
	//打包数据
	axi16550BaudInit(CHOOSE_CHANNEL, baud_rate);

	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}

	return 0;
}


int usart_set_xon_xoff(int client_socket, char *buf, int buf_len)
{
	int ret;

	//字符串比较VSTART和VSTOP从buf[2]开始
	if( (strcmp(&buf[2], "VSTART")) == 0)
	{
		//调用AXI_api设置XonXoff
//		send_xon_xoff_char(CHOOSE_CHANNEL, 1); // 发送 XON
	}
	else if( (strcmp(&buf[2], "VSTOP")) == 0)
	{
//		send_xon_xoff_char(CHOOSE_CHANNEL, 0); // 发送 XOFF
	}

	//返回数据给中间件
	//    ret = socket_send_to_middle(client_socket, "ok", 3);
	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}

	return 0 ;
}


int usart_set_tx_fifo(int client_socket, char *buf, int buf_len)
{
	unsigned fifo_size;
	int ret;

	fifo_size = buf[2];
	//printf("fifo_size: %d\n", fifo_size);

	//调用AXI_api设置TX_FIFO

	// 根据 fifo_size 配置 16550 FCR 寄存器
	unsigned char fcr_value = FCR_FIFO_ENABLE;
	if (fifo_size == 1) {
		fcr_value |= FCR_TRIGGER_LEVEL_1;
	} else if (fifo_size == 16) {
		fcr_value |= FCR_TRIGGER_LEVEL_16;
	}
	userAxiCfgWrite(CHOOSE_CHANNEL, AXI_16550_FCR, fcr_value);

	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	//返回数据给中间件
	//    ret = socket_send_to_middle(client_socket, "ok", 3);
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}

	return 0 ;
}


int usart_set_line_control(int client_socket, char *buf, int buf_len)
{
	int ret;
	unsigned char dtr_val = buf[2];
	unsigned char rts_val = buf[3];

	// 获取当前 MCR 寄存器值
	unsigned int mcr_reg = userAxiCfgRead(CHOOSE_CHANNEL, AXI_16550_MCR);

	// 设置 DTR 位
	if (dtr_val) {
		mcr_reg |= MCR_DTR;
	} else {
		mcr_reg &= ~MCR_DTR;
	}

	// 设置 RTS 位
	if (rts_val) {
		mcr_reg |= MCR_RTS;
	} else {
		mcr_reg &= ~MCR_RTS;
	}

	// 写入更新后的 MCR 寄存器值
	userAxiCfgWrite(CHOOSE_CHANNEL, AXI_16550_MCR, mcr_reg);

	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	//返回数据给中间件
	//    ret = socket_send_to_middle(client_socket, "ok", 3);
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}

	return 0 ;
}

int usart_set_xon(int client_socket, char *buf, int buf_len)
{
	int ret;
//	send_xon_xoff_char(CHOOSE_CHANNEL, 1);
	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	//返回数据给中间件
	//    ret = socket_send_to_middle(client_socket, "ok", 3);
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}

	return 0 ;
}

int usart_set_xoff(int client_socket, char *buf, int buf_len)
{
	int ret;
//	send_xon_xoff_char(CHOOSE_CHANNEL, 0);
	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	//返回数据给中间件
	//    ret = socket_send_to_middle(client_socket, "ok", 3);
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}

	return 0 ;
}


int usart_set_start_break(int client_socket, char *buf, int buf_len)
{
	axi16550SendStartBreak(CHOOSE_CHANNEL);
	int ret;
	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	//返回数据给中间件
	//    ret = socket_send_to_middle(client_socket, "ok", 3);
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}
	return 0 ;

}

int usart_set_stop_break(int client_socket, char *buf, int buf_len)
{
	int ret;
	axi16550SendStopBreak(CHOOSE_CHANNEL);
	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	//返回数据给中间件
	//    ret = socket_send_to_middle(client_socket, "ok", 3);
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}
	return 0 ;

}

int usart_report_hearbeat(int client_socket, char *buf, int buf_len)
{
//	printf("1111\r\n");
	// 启动心跳发送任务
//	heartbeat_task_id = taskSpawn( "heartbeatTask", 100, 0, 20000, (FUNCPTR)heartbeat_send_task, client_socket, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	return 0 ;

}

int usart_report_queue(int client_socket, char *buf, int buf_len)
{
	int ret;

	char pack_buf[4];

	//打包数据
	pack_buf[0] = buf[0];
	pack_buf[1] = 0x02;
	pack_buf[2] = 0x00;
	pack_buf[3] = 0x00;

	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, pack_buf, sizeof(pack_buf));
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}
	return 0 ;

}
int usart_close(int client_socket, char *buf, int buf_len)
{
	int ret;

	char response[3] = {0};
	response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
	response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
	response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
	//返回数据给中间件
	ret = socket_send_to_middle(client_socket, response, sizeof(response));
	//返回数据给中间件
	//    ret = socket_send_to_middle(client_socket, "ok", 3);
	if(ret < 0)
	{
		//printf("send error\n");
		return -1;
	}
	return 0 ;


}

void handle_command(int client_socket, char *buf, int buf_len) 
{

	/*解析数据*/
	unsigned char cmd = buf[0];
	unsigned char data_len = buf[1];

	switch(cmd)
	{
	case ASPP_CMD_PORT_INIT:
	{
		init_usart(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_NOTIFY:
	{

		break;
	}

	case ASPP_CMD_SETBAUD:
	{
		usart_set_baudrate(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_XONXOFF:
	{
		usart_set_xon_xoff(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_TX_FIFO:
	{
		usart_set_tx_fifo(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_LINECTRL:
	{
		usart_set_line_control(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_SETXON:
	{
		usart_set_xon(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_SETXOFF:
	{
		usart_set_xoff(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_START_BREAK:
	{
		usart_set_start_break(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_STOP_BREAK:
	{   
		usart_set_stop_break(client_socket, buf, data_len);

		break;
	}

	case ASPP_CMD_ALIVE:
	{
//		usart_report_hearbeat(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_WAIT_OQUEUE:
	{
		usart_report_queue(client_socket, buf, data_len);
		break;
	}

	case ASPP_CMD_FLUSH:
	{
		usart_close(client_socket, buf, data_len);
		break;
	}


	default:
	{
		//printf("Unknown command: %d\n", cmd);
		break;
	}

	}

}
// 心跳发送任务函数
void heartbeat_send_task(int client_socket) {
	int last_tick = tickGet();         // 获取初始系统节拍
	char pack_buf[3];
	while (1) {
		int current_tick = tickGet();
		// 计算时间间隔（假设系统节拍为 10ms，20 秒对应 2000 个节拍）
		if (current_tick - last_tick >= 2000) { 
			// 打包心跳数据
			pack_buf[0] = ASPP_CMD_POLLING;
			pack_buf[1] = 0x1;
			pack_buf[2] = 0x24;
			// 发送心跳信号
			socket_send_to_middle(client_socket, pack_buf, sizeof(pack_buf));
			last_tick = current_tick;  // 更新上次发送时间
//			printf("current_tick");
		}
		//        printf("current_tick");
		taskDelay(1);  // 让出 CPU 时间片
	}
}

// 任务函数：处理客户端连接
void FixedPortClient(int client_socket) {
	char buffer[BUFFERCOM_SIZE] = {0};
	int valread;

	//printf("New connection: sock %d\n", client_socket);
	set_nonblocking(client_socket);

	//调用AXI_api设置串口相关寄存器
	unsigned char send_buf[4] = {0x26, 0x00, 0x00, 0x81};
	int send_len = sizeof(send_buf);

	if (send(client_socket, send_buf, send_len, 0) != send_len) {
		perror("send failed");
		close(client_socket);
		return;
	}   
	//printf("Sent 0x26000081 to client %d\n", client_socket);
//	unsigned char sendhart_buf[3] = {0x27, 0x01, 0x24};
//	int send_lenhart = sizeof(sendhart_buf);
//	if (send(client_socket, sendhart_buf, send_lenhart, 0) != send_lenhart) {
//		perror("send failed");
//		close(client_socket);
//		return;
//	}  
	while (1) {
		valread = recv(client_socket, buffer, BUFFERCOM_SIZE, 0);
		if (valread == 0) {
			//printf("Client disconnected: sock %d\n", client_socket);
			close(client_socket);
			return;
		} else if (valread == ERROR) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				perror("recv failed");
				close(client_socket);
				return;
			}
			taskDelay(10); // 短暂延时避免忙等待
			continue;
		}

		handle_command(client_socket, buffer, valread);
		memset(buffer, 0, BUFFERCOM_SIZE);
	}
}

// 主任务：TCP 服务器
void FixPortTcpTask() {
	int server_fd, new_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addrlen = sizeof(client_addr);

	// 创建 socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
		perror("socket failed");
		return;
	}

	// 绑定端口
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == ERROR) {
		perror("bind failed");
		close(server_fd);
		return;
	}

	// 监听端口
	if (listen(server_fd, MAXCOM_CLIENTSFD) == ERROR) {
		perror("listen failed");
		close(server_fd);
		return;
	}

	//printf("Server listening on port %d\n", PORT);
	set_nonblocking(server_fd);

	// 主循环：接受连接
	while (1) {
		new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
		if (new_socket == ERROR) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				perror("accept failed");
			}
			taskDelay(10);
			continue;
		}

		// 创建客户端处理任务
		taskSpawn("clientTask", 88, 0, 20000, (FUNCPTR)FixedPortClient, new_socket, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	close(server_fd);
}

// 启动服务器任务
void startPortTcpServer() {
	taskSpawn("tcpServer", 88, 0, 32768, (FUNCPTR)FixPortTcpTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

