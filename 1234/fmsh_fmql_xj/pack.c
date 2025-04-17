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

// 创建socket
void creat_tcp(UART_Config_Params *uart_instance, int port)
{
    uart_instance->clientLen_cmd = sizeof(uart_instance->clientAddr_cmd);
    uart_instance->serverAddr_cmd.sin_family = AF_INET;
    uart_instance->serverAddr_cmd.sin_addr.s_addr = INADDR_ANY;
    uart_instance->serverAddr_cmd.sin_port = htons(port);

    // 创建 socket
    if ((uart_instance->server_fdcmd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        perror("socket failed");
        printf("Failed to create socket for port %d\n", port);
        return;
    }
    printf("Successfully created socket for port %d, socket fd: %d\n", port, uart_instance->server_fdcmd);
}

// 绑定端口
void bind_tcp(UART_Config_Params *uart_instance, int port)
{
    if (bind(uart_instance->server_fdcmd, (struct sockaddr *)&(uart_instance->serverAddr_cmd), sizeof(uart_instance->serverAddr_cmd)) == ERROR) {
        perror("bind failed");
        printf("Failed to bind socket to port %d\n", port);
        close(uart_instance->server_fdcmd);
        return;
    }
    printf("Successfully bound socket to port %d\n", port);
}

// 监听端口
void listen_tcp(UART_Config_Params *uart_instance, int port)
{
    if (listen(uart_instance->server_fdcmd, 16) == ERROR) {
        perror("listen failed");
        printf("Failed to listen on port %d\n", port);
        close(uart_instance->server_fdcmd);
        return;
    }
    int flags = fcntl(uart_instance->server_fdcmd, F_GETFL, 0);
    fcntl(uart_instance->server_fdcmd, F_SETFL, flags | O_NONBLOCK);
    printf("Successfully started listening on port %d\n", port);
}

// 接受连接
void accept_tcp(UART_Config_Params *uart_instance, int port)
{
    while (1) {
        uart_instance->sock_cmd = accept(uart_instance->server_fdcmd, (struct sockaddr *)&(uart_instance->clientAddr_cmd), &(uart_instance->clientLen_cmd));
        if (uart_instance->sock_cmd == ERROR) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("accept failed");
                printf("Failed to accept connection on port %d\n", port);
            }
            taskDelay(10);
            continue;
        }
        printf("Successfully accepted connection on port %d, new socket fd: %d\n", port, uart_instance->sock_cmd);
        recv_uartdata(uart_instance, port);
    }
}

// 接收串口传来的消息
void recv_uartdata(UART_Config_Params *uart_instance, int port)
{
    char buffer[BUFFERCOM_SIZE] = {0};
    while (1) {
        int valread = recv(uart_instance->sock_cmd, buffer, BUFFERCOM_SIZE, 0);
        if (valread == 0) {
            printf("Client disconnected: sock %d on port %d\n", uart_instance->sock_cmd, port);
            close(uart_instance->sock_cmd);
            return;
        } else if (valread == ERROR) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv failed");
                printf("Failed to receive data on port %d\n", port);
                close(uart_instance->sock_cmd);
                return;
            }
            taskDelay(10);
            continue;
        }
        //handle_command(uart_instance->sock_cmd, buffer, valread);
        memset(buffer, 0, BUFFERCOM_SIZE);
    }
}

// 函数声明，接收UART_Config_Params数组指针和数组元素个数
void processUartInstances(UART_Config_Params *uart_instances, int num_ports) {
    for (int i = 0; i < num_ports; i++) {
        int port = portcmd_array[i];
        creat_tcp(&uart_instances[i], port);
        bind_tcp(&uart_instances[i], port);
        listen_tcp(&uart_instances[i], port);

        // 创建一个新任务来接受连接
        TASK_ID taskId = taskSpawn("accept_tcp_task", 90, 0, 40000, (FUNCPTR)accept_tcp, &uart_instances[i], port, 0, 0, 0, 0, 0, 0, 0, 0);
        if (taskId == ERROR) {
            perror("taskSpawn failed");
            printf("Failed to spawn task to accept connections on port %d\n", port);
        } else {
            printf("Successfully spawned task to accept connections on port %d, task ID: %d\n", port, taskId);
        }

        // 示例操作：打印每个实例的sock_cmd_port
        printf("Instance %d: sock_cmd_port = %d\n", i, uart_instances[i].sock_cmd_port);
    }
}

/* 创建一个任务来启动 TCP 服务器 */
void startTcpServerTask() {
    UART_Config_Params uart_instances[NUM_PORTS];
    memset(uart_instances, 0, sizeof(uart_instances));

    TASK_ID taskId = taskSpawn("processUartInstances", 90, 0, 40000, (FUNCPTR)processUartInstances, uart_instances, NUM_PORTS, 0, 0, 0, 0, 0, 0, 0, 0);
    if (taskId == ERROR) {
        perror("taskSpawn failed");
        printf("Failed to spawn task to start TCP server\n");
    } else {
        printf("Successfully spawned task to start TCP server, task ID: %d\n", taskId);
    }
}
