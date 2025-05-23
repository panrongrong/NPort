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
#include <selectLib.h>
#include <tickLib.h>
#include "pack2.h"
#include "oled.h"


// 定义网络配置结构体
typedef struct {
	uint8_t lan;          // 1字节，LAN标识
	uint32_t ip_address;  // 4字节，IP地址
	uint32_t netmask;     // 4字节，子网掩码
	uint32_t gateway;     // 4字节，网关
	uint8_t ip_type;      // 1字节，IP类型标识
}__attribute__((packed)) NetworkConfig;


#define BUFFERNETWORK_SIZE sizeof(NetworkConfig)
#define IPPORT 12345

void parse_network_config(uint8_t *buffer, NetworkConfig *config) {
	// 解析接收到的二进制数据到结构体
	config->lan = buffer[0];
	memcpy(&config->ip_address, buffer + 1, sizeof(uint32_t));
	memcpy(&config->netmask, buffer + 5, sizeof(uint32_t));
	memcpy(&config->gateway, buffer + 9, sizeof(uint32_t));
	config->ip_type = buffer[13];

	// 确保网络字节序转换
	config->ip_address = ntohl(config->ip_address);
	config->netmask = ntohl(config->netmask);
	config->gateway = ntohl(config->gateway);
}


int Configuration_ip() {
    printf("\r\n");
    printf("Configuration_ip service started !\r\n");
    printf("\r\n");
    
    int server_socket, client_socket;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    uint8_t buffer[BUFFER_SIZE];
    NetworkConfig config;

    // 创建socket
    create_tcp_server_socket(&server_socket);

    // 绑定socket到地址和端口
    bind_tcp_server_socket(server_socket, IPPORT);

    // 监听连接
    listen_tcp_server_socket(server_socket);

    printf("Waiting for socket connections...\n");

    // 循环接受客户端连接，允许上位机多次修改配置
    while(1) {
        // 接受客户端连接
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("accept client socket error");
            continue; // 继续等待下一个连接，不退出服务
        }

        printf("Client connected: IP: %s\n", inet_ntoa(client_address.sin_addr));

        // 接收数据
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received == -1) {
            perror("receive data error");
            close(client_socket);
            continue; // 继续等待下一个连接
        }

        // 解析数据
        parse_network_config(buffer, &config);
        
        // 打印解析结果
        printf("LAN: %d\n", config.lan);
        printf("IP Address: %s\n", inet_ntoa(*(struct in_addr *)&config.ip_address));
        printf("Netmask: %s\n", inet_ntoa(*(struct in_addr *)&config.netmask));
        printf("Gateway: %s\n", inet_ntoa(*(struct in_addr *)&config.gateway));
        printf("IP Type: %s\n", config.ip_type == 0 ? "Static" : "DHCP");

        // 打开配置文件
        int fd = open("/mmc0:0/myNewFile", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            printf("Failed to open file: %d\n", fd);
            close(client_socket);
            continue;
        }

        // 生成并配置IP信息
        const char *ip_str = inet_ntoa(*(struct in_addr *)&config.ip_address);
        char ip_strtemp[64];
        
        if(config.lan == 0) {     
            snprintf(ip_strtemp, sizeof(ip_strtemp), "gem0 %s", ip_str); 
            ifconfig("gem0 lladdr 00:01:02:03:04:05");
            ifconfig(ip_strtemp);
            ifconfig("gem0 up");
        } else {
            snprintf(ip_strtemp, sizeof(ip_strtemp), "gem1 %s", ip_str); 
            ifconfig("gem1 lladdr 00:0a:35:11:22:44");    
            ifconfig(ip_strtemp);
            ifconfig("gem1 up");
        }

        // 写入IP信息到文件
        int bytesWritten = write(fd, ip_strtemp, strlen(ip_strtemp));
        if (bytesWritten < 0) {
            printf("Failed to write to file: %d\n", bytesWritten);
        } else {
            printf("Successfully updated network configuration: %s\n", ip_strtemp);
        }
        close(fd);

        // 显示新配置信息
        OLED_Init();
        OLED_ShowString(0, 0, (uint8_t *)"WQXT-UARTSVER-A163LPG", 12, 1);
        OLED_ShowString(0, 20, (uint8_t *)ip_strtemp, 12, 1); 
        OLED_ShowChinese(0, 40, 2, 12, 1);
        OLED_ShowChinese(12, 40, 3, 12, 1);
        OLED_ShowChinesetip(56, 40, 5, 12, 1);
        OLED_ShowChinesetip(68, 40, 6, 12, 1);
        OLED_ShowChinesetip(80, 40, 7, 12, 1);
        OLED_ShowChinesetip(92, 40, 8, 12, 1);
        OLED_ShowChinesetip(104, 40, 9, 12, 1);
        OLED_ShowChinesetip(116, 40, 10, 12, 1);
        OLED_Refresh();
        
        // 保持显示5秒（可根据需要调整）
        taskDelay(500);
        
        // 关闭客户端连接，继续等待下一个请求
        close(client_socket);
    }

    // 不会执行到这里，服务会一直运行
    close(server_socket);
    return 0;
}

void  startmaintask() {
    TASK_ID taskId = taskSpawn("Configuration_ip", 100, 0, 0x4000, (FUNCPTR)Configuration_ip, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (taskId < 0) {
        printf("Failed to spawn task: %d\n", taskId);
    } else {
        printf("Network configuration service started with ID: %d\n", taskId);
    }
}


void setConfig(void)
{
	OLED_Init();
	int fd = open("/mmc0:0/myNewFile",0x202, 0);
	if (fd < 0) {
		printf("Failed to open file: %d\n", fd);
		return;
	}
	char *buf1 = malloc(4096);
	if (!buf1) {
		printf("Failed to allocate memory for buffer\n");
		close(fd);
		return;
	}
	memset(buf1, 0, 4096);
	lseek(fd, 0, SEEK_SET);
	int bytesRead = read(fd, buf1,  4096);
	if (bytesRead < 0) {
		printf("Failed to read from file: %d\n", bytesRead);
		const char* dataToWrite = "gem0 192.168.6.4";
		strncpy(buf1, dataToWrite, strlen(dataToWrite));
		ifconfig("gem0 lladdr 00:01:02:03:04:05");
		ifconfig(dataToWrite);
		ifconfig("gem0 up");
		OLED_Clear();
		OLED_ShowString(0, 0, (uint8_t *)"WQXT-UARTSVER-A163LPG", 12, 1);
		OLED_ShowString(0, 20, (uint8_t *)dataToWrite, 12, 1);  
		OLED_ShowChinese(0,40,0,12,1);//chu
		OLED_ShowChinese(12,40,1,12,1);//shi
		OLED_ShowChinesetip(36,40,0,12,1);
		OLED_ShowChinesetip(48,40,1,12,1);
		OLED_ShowChinesetip(60,40,2,12,1);
		OLED_ShowChinesetip(72,40,3,12,1);
		OLED_ShowChinesetip(84,40,4,12,1);
		OLED_Refresh();
		taskDelay(10);
		int bytesWritten = write(fd, buf1, strlen(dataToWrite));
		if (bytesWritten < 0) {
			printf("Failed to write to file: %d\n", bytesWritten);
			free(buf1);
			close(fd);
			return;

		} else {
			printf("Successfully wrote %d bytes to file.\n", bytesWritten);
			printf("Successfully wrote %s .\n", buf1);
		}

	}else
	{
		printf("Successfully read %d bytes from file.\n", bytesRead);
		buf1[bytesRead] = '\0';
		printf("Data read from file: %s\n", buf1);
		OLED_Clear();
		OLED_ShowString(0, 0, (uint8_t *)"WQXT-UARTSVER-A163LPG", 12, 1);
		OLED_ShowString(0, 20, (uint8_t *)buf1, 12, 1);  
		OLED_ShowChinese(0,40,0,12,1);
		OLED_ShowChinese(12,40,1,12,1);
		OLED_ShowChinesetip(36,40,0,12,1);
		OLED_ShowChinesetip(48,40,1,12,1);
		OLED_ShowChinesetip(60,40,2,12,1);
		OLED_ShowChinesetip(72,40,3,12,1);
		OLED_ShowChinesetip(84,40,4,12,1);
		OLED_Refresh();
		taskDelay(10);
	}

	free(buf1);

	close(fd);
}
void  startsetIptask() {

	taskSpawn("setConfig", 100, 0, 0x2000, (FUNCPTR)setConfig, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

