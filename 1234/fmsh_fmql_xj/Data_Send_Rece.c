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

#define SERVER_PORT 950
// 增大缓冲区大小
#define BUFFER_SIZE 9600
#define MAX_CLIENTSFD 10
#define CHOOSE_CHANNEL 7
// 增大消息队列的最大消息数
#define MAX_MESSAGES 10
#define MAX_MSG_LENGTH 13800
MSG_Q_ID msgQId;
// 串口数据接收线程函数
void uartReceiveTask(int clientSock) {
	uint8_t uartRecvBuf[BUFFER_SIZE];
	uint32_t uartLen;
//	printf("1111\r\n");

	while (1) {
		if (axi16550Recv(CHOOSE_CHANNEL, uartRecvBuf, &uartLen) != -1 && uartLen > 0) {
			for (unsigned int i = 0; i < uartLen; i++) {
//				printf("0x%02X ", uartRecvBuf[i]);
			}
//			printf("\n");
			//            	 taskDelay(1); // 重试前等待
			if (msgQSend(msgQId, (char *)uartRecvBuf, uartLen, NO_WAIT, MSG_PRI_NORMAL) != OK) {
				perror("Failed to send data to message queue");
			}
		}
		taskDelay(1); // 避免忙等待
	}
#if 0    
	while (1) {
		retries = 0;
		while (axi16550Recv(CHOOSE_CHANNEL, uartRecvBuf, &uartLen) != 0 ) {
			taskDelay(1); // 重试前等待
		}

		printf("%s:%d\n\r", __FUNCTION__, __LINE__);
		printf("uartRecvBuf: %s\n", uartRecvBuf);
		printf("uartLen: %d\n", uartLen);

#if 0
		if (uartLen > 0) {
			uint32_t sent = 0;
			while (sent < uartLen) {
				uint32_t chunk = (uartLen - sent > MAX_MSG_LENGTH) ? MAX_MSG_LENGTH : uartLen - sent;
				retries = 0;


				while (msgQSend(msgQId, (char *)uartRecvBuf + sent, chunk, NO_WAIT, MSG_PRI_NORMAL) != OK ) {
					printf("%s:%d\n\r", __FUNCTION__, __LINE__);
					taskDelay(1); // 重试前等待
				}
				sent += chunk;
			}
		}
#endif

		taskDelay(1); // 避免忙等待
	}
#endif        
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
//			printf("msgQReceive recvSize: %d\n", recvSize);
//			printf("msgQReceive buffer: %s\n", recvBuffer);
			while (sent < recvSize) {
				uint32_t chunk = (recvSize - sent > BUFFER_SIZE) ? BUFFER_SIZE : recvSize - sent;
				if (send(clientSock, recvBuffer + sent, chunk, 0) < 0) {
					perror("Failed to send data to client");
					break;
				}
				sent += chunk;
			}
		}

		taskDelay(1);
	}
}

/* 客户端处理任务 */
void clientHandler(int clientSock) {
	char recvBuffer[BUFFER_SIZE];
	int receivedBytes;
	//    MSG_Q_ID msgQId;

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
	taskSpawn("msgQueueHandlerTask", 90, 0, 40000, (FUNCPTR)msgQueueHandlerTask, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);

//	printf("%s:%d\n\r", __FUNCTION__, __LINE__);
	/* 循环接收和发送数据 */
	while (1) {
//		printf("%s:%d\n\r", __FUNCTION__, __LINE__);
		receivedBytes = recv(clientSock, recvBuffer, BUFFER_SIZE - 1, 0);
//		printf("receivedBytes:%ld\r\n",receivedBytes);
		if (receivedBytes < 0) {
			perror("Failed to receive data from client");
			break;
		} else if (receivedBytes == 0) {
			printf("Client disconnected (sock: %d)\n", clientSock);
			break;
		}
//		recvBuffer[receivedBytes] = '\0';  // 确保数据以空字符结束

//		printf("recv: %s\n", recvBuffer);
//		printf("receivedBytes: %d\n", receivedBytes);
//
//		printf("%s:%d\n\r", __FUNCTION__, __LINE__);

		uint32_t sent = 0;
		while (sent < receivedBytes) {
//			printf("%s:%d\n\r", __FUNCTION__, __LINE__);
			uint32_t chunk = (receivedBytes - sent > 256) ? 256 : receivedBytes - sent;
//			printf("chunk:%d\r\n",chunk);
//			taskDelay(1);
			while (axi16550Send(CHOOSE_CHANNEL, (uint8_t *)recvBuffer + sent, chunk) != 0) {

				taskDelay(1); // 重试前等待
			}
//			printf("%s:%d\n\r", __FUNCTION__, __LINE__);

			//            if (retries >= MAX_RETRIES) {
			//                perror("Failed to send data to UART after retries");
			//                break;
			//            }
			sent += chunk;
			

//			// 根据波特率计算延时时间
//			int delayTicks = (chunk * 10 * sysClkRateGet()) / global_baud_rate; // 每个字节 10 位（起始位 + 数据位 + 停止位）
//			taskDelay(delayTicks);
		}
//		printf("sent:%d\r\n",sent);
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
		taskSpawn("clientHandlerTask", 90, 0, 20000, (FUNCPTR)clientHandler, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	/* 关闭服务器 socket */
	close(serverSock);
}

/* 创建一个任务来启动 TCP 服务器 */
void startTcpServerTask() {
	taskSpawn("tcpServerTask", 90, 0, 20000, (FUNCPTR)tcpServerTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}
