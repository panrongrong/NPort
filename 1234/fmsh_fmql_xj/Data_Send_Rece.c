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
// ���󻺳�����С
#define BUFFER_SIZE 9600
#define MAX_CLIENTSFD 10
#define CHOOSE_CHANNEL 7
// ������Ϣ���е������Ϣ��
#define MAX_MESSAGES 10
#define MAX_MSG_LENGTH 13800
MSG_Q_ID msgQId;
// �������ݽ����̺߳���
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
			//            	 taskDelay(1); // ����ǰ�ȴ�
			if (msgQSend(msgQId, (char *)uartRecvBuf, uartLen, NO_WAIT, MSG_PRI_NORMAL) != OK) {
				perror("Failed to send data to message queue");
			}
		}
		taskDelay(1); // ����æ�ȴ�
	}
#if 0    
	while (1) {
		retries = 0;
		while (axi16550Recv(CHOOSE_CHANNEL, uartRecvBuf, &uartLen) != 0 ) {
			taskDelay(1); // ����ǰ�ȴ�
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
					taskDelay(1); // ����ǰ�ȴ�
				}
				sent += chunk;
			}
		}
#endif

		taskDelay(1); // ����æ�ȴ�
	}
#endif        
}

// ��Ϣ���д�������
void msgQueueHandlerTask(int clientSock) {
	char recvBuffer[BUFFER_SIZE];
	int recvSize;
	printf("2222\r\n");
	while (1) {
		recvSize = msgQReceive(msgQId, recvBuffer, MAX_MSG_LENGTH, NO_WAIT);
		if (recvSize > 0) {
			uint32_t sent = 0;
//			recvBuffer[recvSize] = '\0'; // ȷ�������Կ��ַ�����
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

/* �ͻ��˴������� */
void clientHandler(int clientSock) {
	char recvBuffer[BUFFER_SIZE];
	int receivedBytes;
	//    MSG_Q_ID msgQId;

	// ������Ϣ����
	msgQId = msgQCreate(MAX_MESSAGES, MAX_MSG_LENGTH, MSG_Q_FIFO);
	if (msgQId == NULL) {
		perror("Failed to create message queue");
		close(clientSock);
		return;
	}

	// �����������ݽ����߳�
	taskSpawn("uartReceiveTask", 88, 0, 40000, (FUNCPTR)uartReceiveTask, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	// ������Ϣ���д�������
	taskSpawn("msgQueueHandlerTask", 90, 0, 40000, (FUNCPTR)msgQueueHandlerTask, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);

//	printf("%s:%d\n\r", __FUNCTION__, __LINE__);
	/* ѭ�����պͷ������� */
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
//		recvBuffer[receivedBytes] = '\0';  // ȷ�������Կ��ַ�����

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

				taskDelay(1); // ����ǰ�ȴ�
			}
//			printf("%s:%d\n\r", __FUNCTION__, __LINE__);

			//            if (retries >= MAX_RETRIES) {
			//                perror("Failed to send data to UART after retries");
			//                break;
			//            }
			sent += chunk;
			

//			// ���ݲ����ʼ�����ʱʱ��
//			int delayTicks = (chunk * 10 * sysClkRateGet()) / global_baud_rate; // ÿ���ֽ� 10 λ����ʼλ + ����λ + ֹͣλ��
//			taskDelay(delayTicks);
		}
//		printf("sent:%d\r\n",sent);
		memset(recvBuffer,0x00,BUFFER_SIZE);
	}
	/* ���ӹر� */
	close(clientSock);
	// ɾ����Ϣ����
	msgQDelete(msgQId);
}

/* TCP ���������� */
void tcpServerTask() {
	int serverSock, clientSock;
	struct sockaddr_in serverAddr, clientAddr;
	socklen_t clientLen;

	/* ���������� socket */
	serverSock = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSock < 0) {
		perror("Failed to create socket");
		return;
	}
	// ���÷������׽���Ϊ������ģʽ
	int flags = fcntl(serverSock, F_GETFL, 0);
	fcntl(serverSock, F_SETFL, flags | O_NONBLOCK);

	/* ���÷�������ַ */
	bzero((char *)&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVER_PORT);

	/* �� socket */
	if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
		close(serverSock);
		perror("Failed to bind socket");
		return;
	}
	/* ���������������������� */
	if (listen(serverSock, MAX_CLIENTSFD) < 0) {
		close(serverSock);
		perror("Failed to listen on socket");
		return;
	}

	printf("TCP Server listening on port %d\n", SERVER_PORT);

	/* ��ѭ�����ȴ��ͻ������� */
	while (1) {
		clientLen = sizeof(clientAddr);
		clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
		if (clientSock < 0) {
			perror("Failed to accept connection");
			continue;
		}

		// ���ÿͻ����׽���Ϊ������ģʽ
		int client_flags = fcntl(clientSock, F_GETFL, 0);
		fcntl(clientSock, F_SETFL, client_flags | O_NONBLOCK);

		printf("Accepted connection from %s:%d (sock: %d)\n",
				inet_ntoa(clientAddr.sin_addr),
				ntohs(clientAddr.sin_port),
				clientSock);

		/* Ϊÿ���ͻ��˴���һ���������� */
		taskSpawn("clientHandlerTask", 90, 0, 20000, (FUNCPTR)clientHandler, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	/* �رշ����� socket */
	close(serverSock);
}

/* ����һ������������ TCP ������ */
void startTcpServerTask() {
	taskSpawn("tcpServerTask", 90, 0, 20000, (FUNCPTR)tcpServerTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}
