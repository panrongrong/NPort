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
// ���󻺳�����С
#define BUFFER_SIZE (10240UL)
#define MAX_CLIENTSFD 10
#define CHOOSE_CHANNEL 7
// ������Ϣ���е������Ϣ��
#define MAX_MESSAGES 10
#define MAX_MSG_LENGTH 13800
MSG_Q_ID msgQId;

static uint8_t uartRecvBuf[BUFFER_SIZE];

typedef struct {
	usart_params_t config;  // ��������
    MSG_Q_ID msg_queue;           // ��Ϣ����ָ�루ƽ̨���ʵ�֣�

    int sock_cmd;               // Socket�ļ�������
    int sock_data;              // Socket�ļ�������

    uint16_t sock_cmd_port;       // Socket�˿ں�
    uint16_t sock_data_port;       // Socket�˿ں�

    char *tx_buffer;          // ���ͻ�����ָ��
    char *rx_buffer;          // ���ջ�����ָ��

	uint8_t enable;
} UART_Config_Params;




#define MAX_UART_INSTANCES 16
#define BUFFER_SIZE 1024
//����˶˿�    		966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981

// ��ʼ������UARTʵ��
int init_uart_array(UART_Extended_Params uart_array[MAX_UART_INSTANCES]) {
    if (!uart_array) return -1;

    const uint16_t default_ports[MAX_UART_INSTANCES] = {
    		950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960, 961, 962, 963, 964, 965  // Ԥ����˿ں�ʾ��
        // �ɸ���ʵ�����󲹳����˿�...
  
    };

    for (int i = 0; i < MAX_UART_INSTANCES; i++) {
        // ��ʼ�����ò���
        uart_array[i].config = (UART_16550_Config) {
            .baud_rate = 115200,
            .data_bits = DATA_BITS_8,
            .parity = PARITY_NONE,
            .stop_bits = STOP_BITS_1,
            .flow_ctrl = FLOW_CTRL_NONE,
            .fifo_size = 16
        };

        // ��ʼ���������
        uart_array[i].sock_port = default_ports[i];
        uart_array[i].sock_fd = -1;  // ��ʼ��Ϊ��Ч�ļ�������

        // ���仺����
        uart_array[i].tx_buffer = malloc(BUFFER_SIZE);
        uart_array[i].rx_buffer = malloc(BUFFER_SIZE);
        if (!uart_array[i].tx_buffer || !uart_array[i].rx_buffer) {
            // �������ع��ѷ�����ڴ�
            while (i-- > 0) {
                free(uart_array[i].tx_buffer);
                free(uart_array[i].rx_buffer);
            }
            return -2;
        }

        // ��Ϣ���г�ʼ��������ݾ���ʵ�ֲ��䣩
        uart_array[i].msg_queue = NULL;
    }
    return 0;
}

// ����UART�����շ���ʾ���߼���
void process_uart_communication(UART_Extended_Params* uart) {
    if (!uart || uart->sock_fd < 0) return;

    // ʾ������socket�������� -> ����rx_buffer -> ͨ�����ڷ���
    ssize_t received = recv(uart->sock_fd, uart->rx_buffer, BUFFER_SIZE, 0);
    if (received > 0) {
        // ���������Ӵ��ڷ����߼�����write����ʵ�����豸��
        printf("Sending %zd bytes via UART\n", received);
    }

    // ʾ�����Ӵ��ڽ������� -> ����tx_buffer -> ͨ��socket����
    // �����������ݴӴ��ڵ��������α�����ʾ��
    ssize_t uart_data_size = 0; // ʵ��Ӧ�Ӵ��ڶ�ȡ����
    if (uart_data_size > 0) {
        send(uart->sock_fd, uart->tx_buffer, uart_data_size, 0);
    }
}

// �ͷ�������Դ
void cleanup_uart_array(UART_Extended_Params uart_array[MAX_UART_INSTANCES]) {
    if (!uart_array) return;

    for (int i = 0; i < MAX_UART_INSTANCES; i++) {
        // �ر�socket
        if (uart_array[i].sock_fd >= 0) {
            close(uart_array[i].sock_fd);
        }

        // �ͷŻ�����
        free(uart_array[i].tx_buffer);
        free(uart_array[i].rx_buffer);

        // ������Ϣ���У�����ݾ���ʵ�ֲ��䣩
        if (uart_array[i].msg_queue) {
            // ���磺msg_queue_destroy(uart_array[i].msg_queue);
        }
    }
}

// ʹ��ʾ��
int main() {
    UART_Extended_Params uart_instances[MAX_UART_INSTANCES];

    if (init_uart_array(uart_instances) != 0) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }

    // ģ�⴦���һ��UARTʵ��
    process_uart_communication(&uart_instances[0]);

    // �������ʱ������Դ
    cleanup_uart_array(uart_instances);
    return 0;
}























// �������ݽ����̺߳���
void uartReceiveTask(int clientSock) {

	uint32_t uartLen;


	while (1) {
		OLED0_Set();
		if (axi16550Recv(CHOOSE_CHANNEL, uartRecvBuf, &uartLen) != -1 && uartLen > 0) {
			
			

//			            	 taskDelay(5); // ����ǰ�ȴ�
			if (msgQSend(msgQId, (char *)uartRecvBuf, uartLen, NO_WAIT, MSG_PRI_NORMAL) != OK) {
//				printf("uartLen :%d\r\n",uartLen);
				perror("Failed to send data to message queue");

			}
		}
		taskDelay(1); // ����æ�ȴ�
	}
       
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

/* �ͻ��˴������� */
void clientHandler(int clientSock) {
	char recvBuffer[BUFFER_SIZE];
	int receivedBytes;
	//    MSG_Q_ID msgQId;
//	axi16550Init(CHOOSE_CHANNEL, 9600);
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
	taskSpawn("msgQueueHandlerTask", 88, 0, 40000, (FUNCPTR)msgQueueHandlerTask, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	/* ѭ�����պͷ������� */
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

				taskDelay(1); // ����ǰ�ȴ�
			}

			sent += chunk;
			

//			// ���ݲ����ʼ�����ʱʱ��
//			int delayTicks = (chunk * 10 * sysClkRateGet()) / global_baud_rate; // ÿ���ֽ� 10 λ����ʼλ + ����λ + ֹͣλ��
//			taskDelay(delayTicks);
		}
//		printf("sent:%d\r\n",sent);
		OLED1_Clr();
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
		taskSpawn("clientHandlerTask", 89, 0, 20000, (FUNCPTR)clientHandler, clientSock, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	/* �رշ����� socket */
	close(serverSock);
}

/* ����һ������������ TCP ������ */
void startTcpServerTask() {
	taskSpawn("tcpServerTask", 90, 0, 20000, (FUNCPTR)tcpServerTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}
