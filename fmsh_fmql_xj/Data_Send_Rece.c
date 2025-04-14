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
#include <msgQLib.h>
#include "global_baud.h"
#include "oled.h"

#define SERVER_PORT 950
// 增大缓冲区大小
#define BUFFER_SIZE (10240UL)
#define MAX_CLIENTSFD 10
#define CHOOSE_CHANNEL 7
// 增大消息队列的最大消息数
#define MAX_MESSAGES 10
#define MAX_MSG_LENGTH 13800
MSG_Q_ID msgQId;

static uint8_t uartRecvBuf[BUFFER_SIZE];

typedef struct {
	usart_params_t config;  // 基础配置
    MSG_Q_ID msg_queue;           // 消息队列指针（平台相关实现）

    int sock_cmd;               // Socket文件描述符
    int sock_data;              // Socket文件描述符

    uint16_t sock_cmd_port;       // Socket端口号
    uint16_t sock_data_port;       // Socket端口号

    char *tx_buffer;          // 发送缓冲区指针
    char *rx_buffer;          // 接收缓冲区指针

	uint8_t enable;
} UART_Config_Params;




#define MAX_UART_INSTANCES 16
#define BUFFER_SIZE 1024
//命令端端口    		966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981

// 初始化所有UART实例
int init_uart_array(UART_Extended_Params uart_array[MAX_UART_INSTANCES]) {
    if (!uart_array) return -1;

    const uint16_t default_ports[MAX_UART_INSTANCES] = {
    		950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960, 961, 962, 963, 964, 965  // 预定义端口号示例
        // 可根据实际需求补充更多端口...
  
    };

    for (int i = 0; i < MAX_UART_INSTANCES; i++) {
        // 初始化配置参数
        uart_array[i].config = (UART_16550_Config) {
            .baud_rate = 115200,
            .data_bits = DATA_BITS_8,
            .parity = PARITY_NONE,
            .stop_bits = STOP_BITS_1,
            .flow_ctrl = FLOW_CTRL_NONE,
            .fifo_size = 16
        };

        // 初始化网络参数
        uart_array[i].sock_port = default_ports[i];
        uart_array[i].sock_fd = -1;  // 初始化为无效文件描述符

        // 分配缓冲区
        uart_array[i].tx_buffer = malloc(BUFFER_SIZE);
        uart_array[i].rx_buffer = malloc(BUFFER_SIZE);
        if (!uart_array[i].tx_buffer || !uart_array[i].rx_buffer) {
            // 错误处理：回滚已分配的内存
            while (i-- > 0) {
                free(uart_array[i].tx_buffer);
                free(uart_array[i].rx_buffer);
            }
            return -2;
        }

        // 消息队列初始化（需根据具体实现补充）
        uart_array[i].msg_queue = NULL;
    }
    return 0;
}

// 处理UART数据收发（示例逻辑）
void process_uart_communication(UART_Extended_Params* uart) {
    if (!uart || uart->sock_fd < 0) return;

    // 示例：从socket接收数据 -> 存入rx_buffer -> 通过串口发送
    ssize_t received = recv(uart->sock_fd, uart->rx_buffer, BUFFER_SIZE, 0);
    if (received > 0) {
        // 这里可以添加串口发送逻辑（如write到真实串口设备）
        printf("Sending %zd bytes via UART\n", received);
    }

    // 示例：从串口接收数据 -> 存入tx_buffer -> 通过socket发送
    // （假设有数据从串口到达，这里用伪代码表示）
    ssize_t uart_data_size = 0; // 实际应从串口读取数据
    if (uart_data_size > 0) {
        send(uart->sock_fd, uart->tx_buffer, uart_data_size, 0);
    }
}

// 释放所有资源
void cleanup_uart_array(UART_Extended_Params uart_array[MAX_UART_INSTANCES]) {
    if (!uart_array) return;

    for (int i = 0; i < MAX_UART_INSTANCES; i++) {
        // 关闭socket
        if (uart_array[i].sock_fd >= 0) {
            close(uart_array[i].sock_fd);
        }

        // 释放缓冲区
        free(uart_array[i].tx_buffer);
        free(uart_array[i].rx_buffer);

        // 清理消息队列（需根据具体实现补充）
        if (uart_array[i].msg_queue) {
            // 例如：msg_queue_destroy(uart_array[i].msg_queue);
        }
    }
}

// 使用示例
int main() {
    UART_Extended_Params uart_instances[MAX_UART_INSTANCES];

    if (init_uart_array(uart_instances) != 0) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }

    // 模拟处理第一个UART实例
    process_uart_communication(&uart_instances[0]);

    // 程序结束时清理资源
    cleanup_uart_array(uart_instances);
    return 0;
}























// 串口数据接收线程函数
void uartReceiveTask(int clientSock) {

	uint32_t uartLen;


	while (1) {
		OLED0_Set();
		if (axi16550Recv(CHOOSE_CHANNEL, uartRecvBuf, &uartLen) != -1 && uartLen > 0) {
			
			

//			            	 taskDelay(5); // 重试前等待
			if (msgQSend(msgQId, (char *)uartRecvBuf, uartLen, NO_WAIT, MSG_PRI_NORMAL) != OK) {
//				printf("uartLen :%d\r\n",uartLen);
				perror("Failed to send data to message queue");

			}
		}
		taskDelay(1); // 避免忙等待
	}
       
}

// 消息队列处理任务
void msgQueueHandlerTask(int clientSock) {
	char recvBuffer[BUFFER_SIZE];
	int recvSize;
	printf("2222\r\n");
	while (1) {
		recvSize = msgQReceive(msgQId, recvBuffer, MAX_MSG_LENGTH, NO_WAIT);
		if (recvSize > 0) {
			uint32_t sent = 0;
//			recvBuffer[recvSize] = '\0'; // 确保数据以空字符结束

			while (sent < recvSize) {
				uint32_t chunk = (recvSize - sent > BUFFER_SIZE) ? BUFFER_SIZE : recvSize - sent;
				if (send(clientSock, recvBuffer + sent, chunk, 0) < 0) {
					perror("Failed to send data to client");
					break;
				}
				sent += chunk;
				
			}
		}
		OLED0_Clr();
		taskDelay(1);
	}
}

/* 客户端处理任务 */
void clientHandler(int clientSock) {
	char recvBuffer[BUFFER_SIZE];
	int receivedBytes;
	//    MSG_Q_ID msgQId;
//	axi16550Init(CHOOSE_CHANNEL, 9600);
	// 创建消息队列
	msgQId = msgQCreate(MAX_MESSAGES, MAX_MSG_LENGTH, MSG_Q_FIFO);
	if (msgQId == NULL) {
		perror("Failed to create message queue");
		close(clientSock);
		return;
	}

	// 创建串口数据接收线程
	taskSpawn("uartReceiveTask", 88, 0, 40000, (FUNCPTR)uartReceiveTask, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	// 创建消息队列处理任务
	taskSpawn("msgQueueHandlerTask", 88, 0, 40000, (FUNCPTR)msgQueueHandlerTask, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	/* 循环接收和发送数据 */
	while (1) {

		receivedBytes = recv(clientSock, recvBuffer, BUFFER_SIZE - 1, 0);
//		printf("receivedBytes:%ld\r\n",receivedBytes);
		if (receivedBytes < 0) {
			perror("Failed to receive data from client");
			break;
		} else if (receivedBytes == 0) {
			printf("Client disconnected (sock: %d)\n", clientSock);
			break;
		}

		uint32_t sent = 0;
		OLED1_Set();
		while (sent < receivedBytes) {

			uint32_t chunk = (receivedBytes - sent > 256) ? 256: receivedBytes - sent;
//			printf("chunk:%d\r\n",chunk);

			taskDelay(1);
			while (axi16550Send(CHOOSE_CHANNEL, (uint8_t *)recvBuffer + sent, chunk) != 0) {

				taskDelay(1); // 重试前等待
			}

			sent += chunk;
			

//			// 根据波特率计算延时时间
//			int delayTicks = (chunk * 10 * sysClkRateGet()) / global_baud_rate; // 每个字节 10 位（起始位 + 数据位 + 停止位）
//			taskDelay(delayTicks);
		}
//		printf("sent:%d\r\n",sent);
		OLED1_Clr();
		memset(recvBuffer,0x00,BUFFER_SIZE);
	}
	/* 连接关闭 */
	close(clientSock);
	// 删除消息队列
	msgQDelete(msgQId);
}

/* TCP 服务器任务 */
void tcpServerTask() {
	int serverSock, clientSock;
	struct sockaddr_in serverAddr, clientAddr;
	socklen_t clientLen;

	/* 创建服务器 socket */
	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSock < 0) {
		perror("Failed to create socket");
		return;
	}
	// 设置服务器套接字为非阻塞模式
	int flags = fcntl(serverSock, F_GETFL, 0);
	fcntl(serverSock, F_SETFL, flags | O_NONBLOCK);

	/* 配置服务器地址 */
	bzero((char *)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVER_PORT);

	/* 绑定 socket */
	if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		close(serverSock);
		perror("Failed to bind socket");
		return;
	}
	/* 监听并设置最大挂起连接数 */
	if (listen(serverSock, MAX_CLIENTSFD) < 0) {
		close(serverSock);
		perror("Failed to listen on socket");
		return;
	}

	printf("TCP Server listening on port %d\n", SERVER_PORT);

	/* 主循环：等待客户端连接 */
	while (1) {
		clientLen = sizeof(clientAddr);
		clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
		if (clientSock < 0) {
			perror("Failed to accept connection");
			continue;
		}

		// 设置客户端套接字为非阻塞模式
		int client_flags = fcntl(clientSock, F_GETFL, 0);
		fcntl(clientSock, F_SETFL, client_flags | O_NONBLOCK);

		printf("Accepted connection from %s:%d (sock: %d)\n",
				inet_ntoa(clientAddr.sin_addr),
				ntohs(clientAddr.sin_port),
				clientSock);

		/* 为每个客户端创建一个处理任务 */
		taskSpawn("clientHandlerTask", 89, 0, 20000, (FUNCPTR)clientHandler, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	/* 关闭服务器 socket */
	close(serverSock);
}

/* 创建一个任务来启动 TCP 服务器 */
void startTcpServerTask() {
	taskSpawn("tcpServerTask", 90, 0, 20000, (FUNCPTR)tcpServerTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}
