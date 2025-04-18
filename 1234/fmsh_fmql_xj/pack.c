//#include <vxWorks.h>
//#include <stdio.h>
//#include <sockLib.h>
//#include <inetLib.h>
//#include <taskLib.h>
//#include <socket.h>
//#include <stdlib.h>
//#include <sysLib.h>
//#include <logLib.h>
//#include <errno.h>
//#include <string.h>
//#include "wdLib.h"
//#include "in.h"
//#include "ioLib.h"
//#include <unistd.h>
//#include <sys/socket.h>
//#include <stdint.h>
//#include <stddef.h>
//#include <fcntl.h>
//#include <tickLib.h>
//#include <msgQLib.h>
//#include "global_baud.h"
//#include "pack.h"
//
//// 创建socket
//void creat_tcp(UART_Config_Params *uart_instance, int port)
//{
//    uart_instance->clientLen_cmd = sizeof(uart_instance->clientAddr_cmd);
//    uart_instance->serverAddr_cmd.sin_family = AF_INET;
//    uart_instance->serverAddr_cmd.sin_addr.s_addr = INADDR_ANY;
//    uart_instance->serverAddr_cmd.sin_port = htons(port);
//
//    // 创建 socket
//    if ((uart_instance->server_fdcmd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
//        perror("socket failed");
//        printf("Failed to create socket for port %d\n", port);
//        return;
//    }
//    printf("Successfully created socket for port %d, socket fd: %d\n", port, uart_instance->server_fdcmd);
//}
//
//// 绑定端口
//void bind_tcp(UART_Config_Params *uart_instance, int port)
//{
//    if (bind(uart_instance->server_fdcmd, (struct sockaddr *)&(uart_instance->serverAddr_cmd), sizeof(uart_instance->serverAddr_cmd)) == ERROR) {
//        perror("bind failed");
//        printf("Failed to bind socket to port %d\n", port);
//        close(uart_instance->server_fdcmd);
//        return;
//    }
//    printf("Successfully bound socket to port %d\n", port);
//}
//
//// 监听端口
//void listen_tcp(UART_Config_Params *uart_instance, int port)
//{
//    if (listen(uart_instance->server_fdcmd, 16) == ERROR) {
//        perror("listen failed");
//        printf("Failed to listen on port %d\n", port);
//        close(uart_instance->server_fdcmd);
//        return;
//    }
//    int flags = fcntl(uart_instance->server_fdcmd, F_GETFL, 0);
//    fcntl(uart_instance->server_fdcmd, F_SETFL, flags | O_NONBLOCK);
//    printf("Successfully started listening on port %d\n", port);
//}
//
//// 接受连接
//void accept_tcp(UART_Config_Params *uart_instance, int port)
//{
//    while (1) {
//        uart_instance->sock_cmd = accept(uart_instance->server_fdcmd, (struct sockaddr *)&(uart_instance->clientAddr_cmd), &(uart_instance->clientLen_cmd));
//        if (uart_instance->sock_cmd == ERROR) {
//            if (errno != EAGAIN && errno != EWOULDBLOCK) {
//                perror("accept failed");
//                printf("Failed to accept connection on port %d\n", port);
//            }
//            taskDelay(10);
//            continue;
//        }
//        printf("Successfully accepted connection on port %d, new socket fd: %d\n", port, uart_instance->sock_cmd);
//        recv_uartdata(uart_instance, port);
//    }
//}
//
//// 接收串口传来的消息
//void recv_uartdata(UART_Config_Params *uart_instance, int port)
//{
//    char buffer[BUFFERCOM_SIZE] = {0};
//    while (1) {
//        int valread = recv(uart_instance->sock_cmd, buffer, BUFFERCOM_SIZE, 0);
//        if (valread == 0) {
//            printf("Client disconnected: sock %d on port %d\n", uart_instance->sock_cmd, port);
//            close(uart_instance->sock_cmd);
//            return;
//        } else if (valread == ERROR) {
//            if (errno != EAGAIN && errno != EWOULDBLOCK) {
//                perror("recv failed");
//                printf("Failed to receive data on port %d\n", port);
//                close(uart_instance->sock_cmd);
//                return;
//            }
//            taskDelay(10);
//            continue;
//        }
//        //handle_command(uart_instance->sock_cmd, buffer, valread);
//        memset(buffer, 0, BUFFERCOM_SIZE);
//    }
//}
//
//// 函数声明，接收UART_Config_Params数组指针和数组元素个数
//void processUartInstances(UART_Config_Params *uart_instances, int num_ports) {
//    for (int i = 0; i < num_ports; i++) {
//        int port = portcmd_array[i];
//        creat_tcp(&uart_instances[i], port);
//        bind_tcp(&uart_instances[i], port);
//        listen_tcp(&uart_instances[i], port);
//
//        // 创建一个新任务来接受连接
//        TASK_ID taskId = taskSpawn("accept_tcp_task", 90, 0, 40000, (FUNCPTR)accept_tcp, &uart_instances[i], port, 0, 0, 0, 0, 0, 0, 0, 0);
//        if (taskId == ERROR) {
//            perror("taskSpawn failed");
//            printf("Failed to spawn task to accept connections on port %d\n", port);
//        } else {
//            printf("Successfully spawned task to accept connections on port %d, task ID: %d\n", port, taskId);
//        }
//
//        // 示例操作：打印每个实例的sock_cmd_port
//        printf("Instance %d: sock_cmd_port = %d\n", i, uart_instances[i].sock_cmd_port);
//    }
//}
//
///* 创建一个任务来启动 TCP 服务器 */
//void startTcpServerTask() {
////    UART_Config_Params uart_instances[NUM_PORTS];
//    memset(uart_instances, 0, sizeof(uart_instances));
//
//    TASK_ID taskId = taskSpawn("processUartInstances", 90, 0, 40000, (FUNCPTR)processUartInstances, uart_instances, NUM_PORTS, 0, 0, 0, 0, 0, 0, 0, 0);
//    if (taskId == ERROR) {
//        perror("taskSpawn failed");
//        printf("Failed to spawn task to start TCP server\n");
//    } else {
//        printf("Successfully spawned task to start TCP server, task ID: %d\n", taskId);
//    }
//}
/*上面代码实现连接多端口连接tcp*/

//连接可以实现，但是只能一个通道发送数据
//#include <vxWorks.h>
//#include <stdio.h>
//#include <sockLib.h>
//#include <inetLib.h>
//#include <taskLib.h>
//#include <socket.h>
//#include <stdlib.h>
//#include <sysLib.h>
//#include <logLib.h>
//#include <errno.h>
//#include <string.h>
//#include "wdLib.h"
//#include "in.h"
//#include "ioLib.h"
//#include <unistd.h>
//#include <sys/socket.h>
//#include <stdint.h>
//#include <stddef.h>
//#include <fcntl.h>
//#include <tickLib.h>
//#include <msgQLib.h>
//#include "global_baud.h"
//#include "pack.h"
//
//// 创建socket
//void creat_tcp(UART_Config_Params *uart_instance)
//{
//	uart_instance->clientLen_cmd = sizeof(uart_instance->clientAddr_cmd);
//	uart_instance->serverAddr_cmd.sin_family = AF_INET;
//	uart_instance->serverAddr_cmd.sin_addr.s_addr = INADDR_ANY;
//	uart_instance->serverAddr_cmd.sin_port = htons(uart_instance->sock_cmd_port);
//
//	// 创建 socket
//	if ((uart_instance->server_fdcmd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
//		perror("socket failed");
//		printf("Failed to create socket for port %d\n", uart_instance->sock_cmd_port);
//		return;
//	}
//	printf("Successfully created socket for port %d, socket fd: %d\n", uart_instance->sock_cmd_port, uart_instance->server_fdcmd);
//}
//
//void creat_tcpdata(UART_Config_Params *uart_instance)
//{
//	uart_instance->clientLen_data = sizeof(uart_instance->clientAddr_data);
//	uart_instance->serverAddr_data.sin_family = AF_INET;
//	uart_instance->serverAddr_data.sin_addr.s_addr = INADDR_ANY;
//	uart_instance->serverAddr_data.sin_port = htons(uart_instance->sock_data_port);
//
//	// 创建 socket
//	if ((uart_instance->server_fddata = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
//		perror("socket failed");
//		printf("Failed to create socket for port %d\n", uart_instance->sock_data_port);
//		return;
//	}
//	printf("Successfully created socket for port %d, socket fd: %d\n", uart_instance->sock_data_port, uart_instance->server_fddata);
//}
//
//
//// 绑定端口
//void bind_tcp(UART_Config_Params *uart_instance)
//{
//	if (bind(uart_instance->server_fdcmd, (struct sockaddr *)&(uart_instance->serverAddr_cmd), sizeof(uart_instance->serverAddr_cmd)) == ERROR) {
//		perror("bind failed");
//		printf("Failed to bind socket to port %d\n", uart_instance->sock_cmd_port);
//		close(uart_instance->server_fdcmd);
//		return;
//	}
//	printf("Successfully bound socket to port %d\n", uart_instance->sock_cmd_port);
//}
//
//// 绑定端口
//void bind_tcpdata(UART_Config_Params *uart_instance)
//{
//	if (bind(uart_instance->server_fddata, (struct sockaddr *)&(uart_instance->serverAddr_data), sizeof(uart_instance->serverAddr_data)) == ERROR) {
//		perror("bind failed");
//		printf("Failed to bind socket to port %d\n", uart_instance->sock_data_port);
//		close(uart_instance->server_fddata);
//		return;
//	}
//	printf("Successfully bound socket to port %d\n", uart_instance->sock_data_port);
//}
//
//// 监听端口
//void listen_tcp(UART_Config_Params *uart_instance)
//{
//	if (listen(uart_instance->server_fdcmd, 16) == ERROR) {
//		perror("listen failed");
//		printf("Failed to listen on port %d\n", uart_instance->sock_cmd_port);
//		close(uart_instance->server_fdcmd);
//		return;
//	}
//	int flags = fcntl(uart_instance->server_fdcmd, F_GETFL, 0);
//	fcntl(uart_instance->server_fdcmd, F_SETFL, flags | O_NONBLOCK);
//	printf("Successfully started listening on port %d\n", uart_instance->sock_cmd_port);
//}
//
//void listen_tcpdata(UART_Config_Params *uart_instance)
//{
//	if (listen(uart_instance->server_fddata, 16) == ERROR) {
//		perror("listen failed");
//		printf("Failed to listen on port %d\n", uart_instance->sock_data_port);
//		close(uart_instance->server_fddata);
//		return;
//	}
//	int flags = fcntl(uart_instance->server_fddata, F_GETFL, 0);
//	fcntl(uart_instance->server_fddata, F_SETFL, flags | O_NONBLOCK);
//	printf("Successfully started listening on port %d\n", uart_instance->sock_data_port);
//}
//
//// 接受连接
//void accept_tcp(UART_Config_Params *uart_instance)
//{
//	while (1) {
//		uart_instance->sock_cmd = accept(uart_instance->server_fdcmd, (struct sockaddr *)&(uart_instance->clientAddr_cmd), &(uart_instance->clientLen_cmd));
//		if (uart_instance->sock_cmd == ERROR) {
//			if (errno != EAGAIN && errno != EWOULDBLOCK) {
//				perror("accept failed");
//				printf("Failed to accept connection on port %d\n", uart_instance->sock_cmd_port);
//			}
//			taskDelay(1);
//			continue;
//		}
//		printf("Successfully accepted connection  new socket fd: %d\n", uart_instance->sock_cmd);
//	}
//}
//
//void accept_tcpdata(UART_Config_Params *uart_instance)
//{
//	while (1) {
//		uart_instance->sock_data = accept(uart_instance->server_fddata, (struct sockaddr *)&(uart_instance->clientAddr_data), &(uart_instance->clientLen_data));
//		if (uart_instance->sock_data == ERROR) {
//			if (errno != EAGAIN && errno != EWOULDBLOCK) {
//				perror("accept failed");
//				printf("Failed to accept connection on port %d\n", uart_instance-> sock_data_port);
//			}
//			taskDelay(1);
//			continue;
//		}
//		printf("Successfully accepted connection  new socket fd: %d\n", uart_instance->sock_data);
//		recv_tcpdata(uart_instance);
//	}
//}
//
//void recv_tcpdata(UART_Config_Params *uart_instance)
//{
//    char buffer[BUFFERCOM_SIZE] = {0};
//    int receivedBytes;
//    while(1){
//        receivedBytes = recv(uart_instance->sock_data, buffer, BUFFERCOM_SIZE, 0);
//        if (receivedBytes < 0) {
//            perror("Failed to receive data from client");
//            printf("errno: %d\n", errno);
//        } else if (receivedBytes == 0) {
//            printf("Client disconnected (sock: %d)\n", uart_instance->sock_data);
//        } else {
//            printf("receivedBytes:%ld\r\n", (long)receivedBytes);
//        }
//        taskDelay(1);
//        if (receivedBytes > 0) {
//            if (axi16550Send(uart_instance->choose_channel, (uint8_t *)buffer, receivedBytes) == 0){
//                printf("Sent data to UART channel %d: %s\n", uart_instance->choose_channel, buffer);
//                taskDelay(1); // 重试前等待
//            }
//        }
//    }
//    close(uart_instance->sock_data);
//}
//void recv_aurtdata(UART_Config_Params *uart_instance)
//{
//	uint8_t uartRecvBuf[BUFFERCOM_SIZE];
//	uint32_t uartLen;
//	while (1) {
//		if (axi16550Recv(uart_instance->choose_channel, uartRecvBuf, &uartLen) != -1 && uartLen > 0) {
//			//printf("Received data from UART channel %d: ", CHOOSE_CHANNEL);
//			for (unsigned int i = 0; i < uartLen; i++) {
//				printf("0x%02X ", uartRecvBuf[i]);
//			}
//			printf("\n");
//			// 将 UART 接收到的数据发送回客户端
//			if (send(uart_instance->sock_data, uartRecvBuf, uartLen, 0) < 0) {
//				printf("Failed to send data to client (sock: %d) from UART channel %d\n", uart_instance->sock_data, uart_instance->choose_channel);
//			} else {
//				printf("Success to send data to client (sock: %d) from UART channel %d\n", uart_instance->sock_data, uart_instance->choose_channel);
//			}
//		}
//		taskDelay(1); // 避免忙等待
//	}
//}
//// 函数声明，接收UART_Config_Params数组指针和数组元素个数
//void processUartInstances(UART_Config_Params *uart_instances, int num_ports) {
//	int i;
//	for (i =0; i < num_ports; i++) {
//		uart_instances[i].sock_data_port = portdata_array[i];
//		uart_instances[i].sock_cmd_port = portcmd_array[i];
//		uart_instances[i].choose_channel = i;
//		//        int port = portcmd_array[i];
//		creat_tcp(&uart_instances[i]);
//		bind_tcp(&uart_instances[i]);
//		listen_tcp(&uart_instances[i]);
//
//		creat_tcpdata(&uart_instances[i]);
//		bind_tcpdata(&uart_instances[i]);
//		listen_tcpdata(&uart_instances[i]);
//		// 创建一个新任务来接受连接
//		TASK_ID taskId = taskSpawn("accept_tcp_task", 90, 0, 40000, (FUNCPTR)accept_tcp, &uart_instances[i], 0, 0, 0, 0, 0, 0, 0, 0, 0);
//		if (taskId == ERROR) {
//			perror("taskSpawn failed");
//		} else {
//			printf("Successfully spawned task to accept connections task ID: %d\n", taskId);
//		}
//
//		// 示例操作：打印每个实例的sock_cmd_port
//		printf("Instance %d: sock_cmd_port = %d\n", i, uart_instances[i].sock_data_port);
//
//		TASK_ID taskdataId = taskSpawn("accept_tcpdata_task", 90, 0, 40000, (FUNCPTR)accept_tcpdata, &uart_instances[i], 0, 0, 0, 0, 0, 0, 0, 0, 0);
//		if (taskdataId == ERROR) {
//			perror("taskSpawn failed");
//		} else {
//			printf("Successfully spawned task to accept connections task ID: %d\n", taskdataId);
//		}
//
//		TASK_ID recvTaskId = taskSpawn("recv_aurtdata_task", 90, 0, 40000, (FUNCPTR)recv_aurtdata, &uart_instances[i], 0, 0, 0, 0, 0, 0, 0, 0, 0);
//		if (recvTaskId == ERROR) {
//			perror("taskSpawn for recv_aurtdata failed");
//		} else {
//			printf("Successfully spawned task for recv_aurtdata, task ID: %d\n", recvTaskId);
//		}
//		// 示例操作：打印每个实例的sock_cmd_port
//		printf("Instance %d: sock_cmd_port = %d\n", i, uart_instances[i].sock_data_port);
//	}
//}
//
///* 创建一个任务来启动 TCP 服务器 */
//void startTcpServerTask() {
//	//    UART_Config_Params uart_instances[NUM_PORTS];
//	memset(uart_instances, 0, sizeof(uart_instances));
//
//	TASK_ID taskId = taskSpawn("processUartInstances", 90, 0, 40000, (FUNCPTR)processUartInstances, uart_instances, NUM_PORTS, 0, 0, 0, 0, 0, 0, 0, 0);
//	if (taskId == ERROR) {
//		perror("taskSpawn failed");
//		printf("Failed to spawn task to start TCP server\n");
//	} else {
//		printf("Successfully spawned task to start TCP server, task ID: %d\n", taskId);
//	}
//}


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
#include "pack.h"

// 创建命令通道Socket
void creat_tcp(UART_Config_Params *uart_instance) {
	uart_instance->clientLen_cmd = sizeof(uart_instance->clientAddr_cmd);
	uart_instance->serverAddr_cmd.sin_family = AF_INET;
	uart_instance->serverAddr_cmd.sin_addr.s_addr = INADDR_ANY;
	uart_instance->serverAddr_cmd.sin_port = htons(uart_instance->sock_cmd_port);

	if ((uart_instance->server_fdcmd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
		perror("[CMD] socket failed");
		return;
	}
	printf("[CMD] Created socket fd:%d port:%d\n", 
			uart_instance->server_fdcmd, 
			uart_instance->sock_cmd_port);
}

// 创建数据通道Socket
void creat_tcpdata(UART_Config_Params *uart_instance) {
	uart_instance->clientLen_data = sizeof(uart_instance->clientAddr_data);
	uart_instance->serverAddr_data.sin_family = AF_INET;
	uart_instance->serverAddr_data.sin_addr.s_addr = INADDR_ANY;
	uart_instance->serverAddr_data.sin_port = htons(uart_instance->sock_data_port);

	if ((uart_instance->server_fddata = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
		perror("[DATA] socket failed");
		return;
	}
	printf("[DATA] Created socket fd:%d port:%d\n", 
			uart_instance->server_fddata, 
			uart_instance->sock_data_port);
}

// 绑定命令通道
void bind_tcp(UART_Config_Params *uart_instance) {
	if (bind(uart_instance->server_fdcmd, 
			(struct sockaddr *)&(uart_instance->serverAddr_cmd),
			sizeof(uart_instance->serverAddr_cmd)) == ERROR) {
		close(uart_instance->server_fdcmd);
		perror("[CMD] bind failed");
		return;
	}
	printf("[CMD] Bound port:%d\n", uart_instance->sock_cmd_port);
}

// 绑定数据通道
void bind_tcpdata(UART_Config_Params *uart_instance) {
	if (bind(uart_instance->server_fddata,
			(struct sockaddr *)&(uart_instance->serverAddr_data),
			sizeof(uart_instance->serverAddr_data)) == ERROR) {
		close(uart_instance->server_fddata);
		perror("[DATA] bind failed");
		return;
	}
	printf("[DATA] Bound port:%d\n", uart_instance->sock_data_port);
}

// 设置非阻塞模式
void set_nonblock(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 监听命令通道
void listen_tcp(UART_Config_Params *uart_instance) {
	if (listen(uart_instance->server_fdcmd, 16) == ERROR) {
		close(uart_instance->server_fdcmd);
		perror("[CMD] listen failed");
		return;
	}
	set_nonblock(uart_instance->server_fdcmd);
	printf("[CMD] Listening port:%d\n", uart_instance->sock_cmd_port);
}

// 监听数据通道
void listen_tcpdata(UART_Config_Params *uart_instance) {
	if (listen(uart_instance->server_fddata, 16) == ERROR) {
		close(uart_instance->server_fddata);
		perror("[DATA] listen failed");
		return;
	}
	set_nonblock(uart_instance->server_fddata);
	printf("[DATA] Listening port:%d\n", uart_instance->sock_data_port);
}

// 命令通道接受连接（独立任务）
void accept_tcp_task(UART_Config_Params *uart_instance) {
	while (1) {
		int new_sock = accept(uart_instance->server_fdcmd,
				(struct sockaddr *)&(uart_instance->clientAddr_cmd),
				&(uart_instance->clientLen_cmd));
		if (new_sock == ERROR) {
			if (errno != EAGAIN) {
				perror("[CMD] accept error");
			}
			taskDelay(1);
			continue;
		}
		uart_instance->sock_cmd = new_sock;
		printf("[CMD] New connection fd:%d port:%d\n", 
				new_sock, 
				uart_instance->sock_cmd_port);
		// 创建专用数据处理任务
		taskSpawn("recv_tcpcmd_task", 90, 0, 40000, 
				(FUNCPTR)recv_tcpcmd_task,  (long)(void *)uart_instance, 0,0,0,0,0,0,0,0,0);
	}
}

// 数据通道接受连接（独立任务）
void accept_tcpdata_task(UART_Config_Params *uart_instance) {
	while (1) {
		int new_sock = accept(uart_instance->server_fddata,
				(struct sockaddr *)&(uart_instance->clientAddr_data),
				&(uart_instance->clientLen_data));
		if (new_sock == ERROR) {
			if (errno != EAGAIN) {
				perror("[DATA] accept error");
			}
			taskDelay(1);
			continue;
		}
		uart_instance->sock_data = new_sock;
		printf("[DATA] New connection fd:%d port:%d\n", 
				new_sock, 
				uart_instance->sock_data_port);

		// 创建专用数据处理任务
		taskSpawn("data_recv_task", 90, 0, 40000, 
				(FUNCPTR)recv_tcpdata_task,  (long)(void *)uart_instance, 0,0,0,0,0,0,0,0,0);
	}
}

// 数据接收处理任务
void recv_tcpdata_task(UART_Config_Params *uart_instance) {
	//	char buffer[BUFFERCOM_SIZE];
	//	char bufferdata[BUFFERCOM_SIZE];
	while(1) {
		int received = recv(uart_instance->sock_data, uart_instance->rx_buffer, BUFFERCOM_SIZE, 0);

		if (received > 0) {
			// 将数据放入消息队列
			if (msgQSend(uart_instance->msg_queue, (char *)uart_instance->rx_buffer, received, NO_WAIT, MSG_PRI_NORMAL) != OK) {
				perror("[DATA] msgQSend failed");
			}
		} 
		else if (received == 0) {
			printf("[DATA] Client disconnected\n");
			close(uart_instance->sock_data);
			break;
		}
		int length = msgQReceive(uart_instance->msg_queue, uart_instance->rx_buffer, BUFFERCOM_SIZE, NO_WAIT);
		if (length > 0) {
			if (axi16550Send(uart_instance->choose_channel, 
					(uint8_t *)uart_instance->rx_buffer, 
					length) == 0) {
				//				printf("[UART] Sent to ch%d: %.*s\n", 
				//						uart_instance->choose_channel, 
				//						length, uart_instance->rx_buffer);
			}
		}
		taskDelay(1);
	}
}

void recv_tcpcmd_task(UART_Config_Params *uart_instance) {
	char cmdbuffer[BUFFERCOM_SIZE];
	//调用AXI_api设置串口相关寄存器
	unsigned char send_buf[4] = {0x26, 0x00, 0x00, 0x81};
	int send_len = sizeof(send_buf);

	if (send(uart_instance->sock_cmd, send_buf, send_len, 0) != send_len) {
		perror("send failed");
		close(uart_instance->sock_cmd);
		return;
	}   
	printf("Sent 0x26000081 to client %d\n", uart_instance->sock_cmd);
	while(1) {
		int received = recv(uart_instance->sock_cmd, cmdbuffer, BUFFERCOM_SIZE, 0);
		if (received == 0) {
			printf("[DATA] Client disconnected\n");
			close(uart_instance->sock_data);
			return;
		} else if (received == ERROR) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				perror("recv failed");
				close(uart_instance->sock_data);
				return;
			}
			taskDelay(10); // 短暂延时避免忙等待
			continue;
		}
		handle_command(uart_instance, uart_instance->sock_cmd, cmdbuffer, received);
		memset(cmdbuffer, 0, BUFFERCOM_SIZE);
	}
}
// UART数据接收任务
void recv_uartdata_task(UART_Config_Params *uart_instance) {
	//	uint8_t buffer[BUFFERCOM_SIZE];
	uint32_t length;

	while(1) {
		if (axi16550Recv(uart_instance->choose_channel, (uint8_t*)uart_instance->tx_buffer, &length) == 0) {
			if (send(uart_instance->sock_data, uart_instance->tx_buffer, length, 0) < 0) {
				printf("[UART] Send failed on ch%d\n", uart_instance->choose_channel);
			}
		}
		taskDelay(1);
	}
}

// 初始化所有实例
void processUartInstances(UART_Config_Params *uart_instances, int num_ports) {
	for (int i = 0; i < num_ports; i++) {
		// 初始化端口配置
		uart_instances[i].sock_cmd_port = portcmd_array[i];
		uart_instances[i].sock_data_port = portdata_array[i];
		uart_instances[i].choose_channel = i;

		// 创建消息队列
		uart_instances[i].msg_queue = msgQCreate(16, BUFFERCOM_SIZE, MSG_Q_FIFO);
		if (uart_instances[i].msg_queue == NULL) {
			perror("[UART] msgQCreate failed");
			continue;
		}

		// 命令通道初始化
		creat_tcp(&uart_instances[i]);
		bind_tcp(&uart_instances[i]);
		listen_tcp(&uart_instances[i]);

		//		axi16550Init(uart_instances[i].choose_channel, 921600);
		// 数据通道初始化
		creat_tcpdata(&uart_instances[i]);
		bind_tcpdata(&uart_instances[i]);
		listen_tcpdata(&uart_instances[i]);



		// 创建任务
		taskSpawn("cmd_accept", 90, 0, 40000, 
				(FUNCPTR)accept_tcp_task,  (long)(void *)&uart_instances[i], 0,0,0,0,0,0,0,0,0);

		taskSpawn("data_accept", 90, 0, 40000, 
				(FUNCPTR)accept_tcpdata_task,  (long)(void *)&uart_instances[i], 0,0,0,0,0,0,0,0,0);

		taskSpawn("uart_recv", 90, 0, 40000, 
				(FUNCPTR)recv_uartdata_task,  (long)(void *)&uart_instances[i], 0,0,0,0,0,0,0,0,0);
	}
}

// 启动服务器主函数
void startTcpServer() {
	taskSpawn("processUartInstances", 90, 0, 40000, 
			(FUNCPTR)processUartInstances,  (long)(void *)uart_instances,NUM_PORTS,0,0,0,0,0,0,0,0);
}
