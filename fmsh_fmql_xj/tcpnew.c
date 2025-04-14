//#include <vxWorks.h>
//#include <sockLib.h>
//#include <inetLib.h>
//#include <taskLib.h>
//#include <fcntl.h>
//#include <ioLib.h>
//#include <string.h>
//#include <stdint.h>
////#include "demo.c"
//
//#define PORT 966
//#define BUFFER_SIZE 1024
//#define MAX_CLIENTSFD 5
//
//#define     ASPP_CMD_PORT_INIT          (0x2c)  //初始化串口
//#define     ASPP_CMD_NOTIFY             (0x26)  //向中间件通知设备状态改变
//#define     ASPP_CMD_SETBAUD            (0x17)  //设置串口波特率
//#define     ASPP_CMD_XONXOFF            (0x18)  //XonXoff
//#define     ASPP_CMD_TX_FIFO            (0x30)  //TX_FIFO
//#define     ASPP_CMD_LINECTRL           (0x12)  //LINECTRL
//#define     ASPP_CMD_SETXON             (0x33)  //SETXON
//#define     ASPP_CMD_SETXOFF            (0x34)  //SETXOFF
//#define     ASPP_CMD_START_BREAK        (0x21)  //START_BREAK
//#define     ASPP_CMD_STOP_BREAK         (0x22)  //STOP_BREAK
//#define     ASPP_CMD_POLLING            (0x27 )  //POLLING
//#define     ASPP_CMD_ALIVE              (0x28)  //ALIVE
//#define     ASPP_CMD_WAIT_OQUEUE        (0x2f)  //WAIT_OQUEUE
//#define     ASPP_CMD_FLUSH              (0x14)  //FLUSH 
//
//#define ASPP_NOTIFY_PARITY   0x01
//#define ASPP_NOTIFY_FRAMING  0x02
//#define ASPP_NOTIFY_HW_OVERRUN  0x04
//#define ASPP_NOTIFY_SW_OVERRUN  0x08
//#define ASPP_NOTIFY_BREAK   0x10
//#define ASPP_NOTIFY_MSR_CHG  0x20
//
//#define UART_MSR_CTS  0x10
//#define UART_MSR_DSR  0x20
//#define UART_MSR_DCD  0x80
//
///**************************gloable variable *****************************************/
//
//
///************************** usart *****************************************/
//#define    USART_STOP_BIT_2   0x02
//#define    USART_STOP_BIT_1   0x01
//
//#define    USART_PARITY_NONE  0x00
//#define    USART_PARITY_EVEN  0x02
//#define    USART_PARITY_ODD   0x01
//
//
//#define    LINE_CONTROL_UART_MCR_DTR 0
//#define    LINE_CONTROL_UART_MCR_RTS 1
//
//
//
//typedef struct usart_params
//{
//    unsigned int  baud_rate;
//    unsigned char data_bit;
//    unsigned char stop_bit;
//    unsigned char parity;
//    unsigned char mark;
//    unsigned char space;
//    unsigned char usart_mcr_dtr;
//    unsigned char usart_mcr_rts;
//    unsigned char xon_xoff; //XonXoff
//
//}usart_params_t;
//
//
//
//int bauderate_table[] = { 300, 600, 1200, 2400, 4800, 7200, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 150, 134, 110, 75, 50};
//unsigned int data_bit_table[] = { 5, 6, 7, 8 };
//
///************************** usart *****************************************/
//
//
///**************************gloable variable *****************************************/
//
//
//void set_nonblocking(int sockfd) {
//    int flags = fcntl(sockfd, F_GETFL, 0);
//    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
//}
//
//int socket_send_to_middle(int sock_fd, char *buf, int buf_len) 
//{
//    for(int i=0; i<buf_len; i++)
//    {
//         printf("buf[%d]:%x\n", i, buf[i]);
//    }
//    
//    int ret = send(sock_fd, buf, buf_len, 0);
//    if (ret < 0) {
//        perror("send");
//        return -1;
//    }
//    return 0;
//}
//
////打包数据
//void pack_data(char *buf, char *pack_buf)
//{
//    pack_buf[0] = buf[0];
//}
//
//int  init_usart(int client_socket, char *buf, int buf_len) 
//{
//    int ret;
//
//    unsigned char stop_bit;
//    unsigned char data_bit;
//    unsigned char parity;
//    unsigned char line_control;
//    unsigned char xon_xoff;
//
//    usart_params_t usart_params;
//
//    char pack_buf[5] = {0};
//    //提取串口参数
//   
//    //设置串口波特率
//    int baud_rate = bauderate_table[(int)buf[2]];
//    usart_params.baud_rate = baud_rate;
//    printf("baud_rate: %d\n", baud_rate);
//
//    //data bit
//    data_bit = (int)buf[3];
//    usart_params.data_bit = data_bit;
//    printf("data_bit: %d\n", data_bit);
//
//    //stop bit
//    if(buf[4] == 0)
//    {
//        stop_bit = USART_STOP_BIT_1;
//        printf("stop_bit: %d\n", stop_bit);
//    }
//    else
//    {
//        stop_bit = USART_STOP_BIT_2;
//        printf("stop_bit: %d\n", stop_bit);
//    }
//    usart_params.stop_bit = stop_bit;
//
//    //parity
//    if(buf[5] == 0)
//    {
//        parity = USART_PARITY_NONE;
//        printf("parity: %d\n", parity);
//    }
//    else if(buf[5] == 8)
//    {
//        parity = USART_PARITY_EVEN;
//        printf("parity: %d\n", parity);
//    }
//    else if(buf[5] == 16)
//    {
//        parity = USART_PARITY_ODD;
//        printf("parity: %d\n", parity);
//    }
//
//    usart_params.parity = parity;
//  
//    //设置XonXoff
//    xon_xoff = buf[9];
//    usart_params.xon_xoff = xon_xoff;
//    printf("xon_xoff: %d\n", xon_xoff);
//
//    
//    //调用AXI_api设置串口相关寄存器
//
//    //打包数据
//    pack_buf[0] = buf[0];
//    pack_buf[1] = 0x3;
//    pack_buf[2] = 0x0;
//    pack_buf[3] = 0x0;
//    pack_buf[4] = 0x0; 
//    //返回数据给中间件
//    ret = socket_send_to_middle(client_socket, pack_buf, sizeof(pack_buf));
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//
//    return 0;
//    
//}
//
//int  usart_set_baudrate(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//    unsigned int baud_rate;
//    
//
//    //提取波特率:假设高位在前
//    baud_rate = buf[2] << 24 | buf[3] << 16 | buf[4] << 8 | buf[5];
//    printf("baud_rate: %d\n", baud_rate);
//
//    //调用AXI_api设置串口波特率
//    //打包数据
//
//    char response[3] = {0};
//       response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//       response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//       response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//    //返回数据给中间件
//    ret = socket_send_to_middle(client_socket, response, sizeof(response));
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//
//    return 0;
//}
//
//
//int usart_set_xon_xoff(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//
//    //字符串比较VSTART和VSTOP从buf[2]开始
//    if( (strcmp(&buf[2], "VSTART")) == 0)
//    {
//        //调用AXI_api设置XonXoff
//    }
//    else if( (strcmp(&buf[2], "VSTOP")) == 0)
//    {
//
//    }
//
//   //返回数据给中间件
////    ret = socket_send_to_middle(client_socket, "ok", 3);
//    char response[3] = {0};
//           response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//           response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//           response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//        //返回数据给中间件
//        ret = socket_send_to_middle(client_socket, response, sizeof(response));
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//
//    return 0 ;
//}
//
//
//int usart_set_tx_fifo(int client_socket, char *buf, int buf_len)
//{
//    unsigned fifo_size;
//    int ret;
//
//    fifo_size = buf[2];
//    printf("fifo_size: %d\n", fifo_size);
//
//    //调用AXI_api设置TX_FIFO
//
//
//    char response[3] = {0};
//                   response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//                   response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//                   response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//                //返回数据给中间件
//                ret = socket_send_to_middle(client_socket, response, sizeof(response));
//        //返回数据给中间件
//    //    ret = socket_send_to_middle(client_socket, "ok", 3);
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//
//    return 0 ;
//
//}
//
//int usart_set_line_control(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//
//    char response[3] = {0};
//               response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//               response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//               response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//            //返回数据给中间件
//            ret = socket_send_to_middle(client_socket, response, sizeof(response));
//   //返回数据给中间件
////    ret = socket_send_to_middle(client_socket, "ok", 3);
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//
//    return 0 ;
//}
//
//
//int usart_set_xon(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//    char response[3] = {0};
//               response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//               response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//               response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//            //返回数据给中间件
//            ret = socket_send_to_middle(client_socket, response, sizeof(response));
//   //返回数据给中间件
////    ret = socket_send_to_middle(client_socket, "ok", 3);
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//
//    return 0 ;
//}
//
//int usart_set_xoff(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//    char response[3] = {0};
//               response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//               response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//               response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//            //返回数据给中间件
//            ret = socket_send_to_middle(client_socket, response, sizeof(response));
//   //返回数据给中间件
////    ret = socket_send_to_middle(client_socket, "ok", 3);
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//
//    return 0 ;
//}
//
//#if 0
////发送break信号函数
//void axi16550SendBreak(unsigned int channel) {
//    unsigned int lcr = userAxiCfgRead(channel, AXI_16550_LCR);
//    
//    // 使能break信号
//    lcr |= (1 << 6);
//    userAxiCfgWrite(channel, AXI_16550_LCR, lcr);
//    
//    // 保持break状态至少一个字符时间（示例1ms）
//    taskDelay(1);
//    
//    // 清除break信号
//    lcr &= ~(1 << 6);
//    userAxiCfgWrite(channel, AXI_16550_LCR, lcr);
//}
//#endif
//int usart_set_start_break(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//    char response[3] = {0};
//               response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//               response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//               response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//            //返回数据给中间件
//            ret = socket_send_to_middle(client_socket, response, sizeof(response));
//    //返回数据给中间件
////    ret = socket_send_to_middle(client_socket, "ok", 3);
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//    return 0 ;
//
//}
//
//int usart_set_stop_break(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//
//    char response[3] = {0};
//                   response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//                   response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//                   response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//                //返回数据给中间件
//                ret = socket_send_to_middle(client_socket, response, sizeof(response));
//        //返回数据给中间件
//    //    ret = socket_send_to_middle(client_socket, "ok", 3);
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//    return 0 ;
//
//}
//
//int usart_report_hearbeat(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//
//    char pack_buf[2];
//
//   //打包数据
//    pack_buf[0] = ASPP_CMD_ALIVE;
//    pack_buf[1] = 0x0;
//    
//    //返回数据给中间件
//    ret = socket_send_to_middle(client_socket, pack_buf, sizeof(pack_buf));
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//    return 0 ;
//
//}
//
//int usart_report_queue(int client_socket, char *buf, int buf_len)
//{
//    int ret;
//
//    char pack_buf[4];
//
//   //打包数据
//    pack_buf[0] = buf[0];
//    pack_buf[1] = 0x02;
//    pack_buf[2] = 0x00;
//    pack_buf[3] = 0x00;
//    
//    //返回数据给中间件
//    ret = socket_send_to_middle(client_socket, pack_buf, sizeof(pack_buf));
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//    return 0 ;
//
//}
//int usart_close(int client_socket, char *buf, int buf_len)
//{
//   int ret;
//
//   char response[3] = {0};
//                  response[0] = buf[0];         // 第一个字节为命令号（如 0x17）
//                  response[1] = 'O';            // 'O' 的 ASCII 码：0x4F
//                  response[2] = 'K';            // 'K' 的 ASCII 码：0x4B
//               //返回数据给中间件
//               ret = socket_send_to_middle(client_socket, response, sizeof(response));
//       //返回数据给中间件
//   //    ret = socket_send_to_middle(client_socket, "ok", 3);
//    if(ret < 0)
//    {
//        printf("send error\n");
//        return -1;
//    }
//    return 0 ;
//
//
//}
//
//
//
//void handle_command(int client_socket, char *buf, int buf_len) 
//{
//
//    /*解析数据*/
//    unsigned char cmd = buf[0];
//    unsigned char data_len = buf[1];
//    
//    switch(cmd)
//    {
//        case ASPP_CMD_PORT_INIT:
//        {
//            init_usart(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_NOTIFY:
//        {
//            
//            break;
//        }
//
//        case ASPP_CMD_SETBAUD:
//        {
//            usart_set_baudrate(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_XONXOFF:
//        {
//            usart_set_xon_xoff(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_TX_FIFO:
//        {
//            usart_set_tx_fifo(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_LINECTRL:
//        {
//            usart_set_line_control(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_SETXON:
//        {
//            usart_set_xon(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_SETXOFF:
//        {
//            usart_set_xoff(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_START_BREAK:
//        {
//            usart_set_start_break(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_STOP_BREAK:
//        {   
//            usart_set_stop_break(client_socket, buf, data_len);
//
//            break;
//        }
//
//        case ASPP_CMD_POLLING:
//        {
//            usart_report_hearbeat(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_WAIT_OQUEUE:
//        {
//        	usart_report_queue(client_socket, buf, data_len);
//            break;
//        }
//
//        case ASPP_CMD_FLUSH:
//        {
//            usart_close(client_socket, buf, data_len);
//            break;
//        }
//
//
//        default:
//        {
//            printf("Unknown command: %d\n", cmd);
//            break;
//        }
//            
//
//
//    }
//
//
//}
//
//
////int main() {
////    int server_fd, new_socket;
////    struct sockaddr_in address;
////    int addrlen = sizeof(address);
////
////    char buffer[BUFFER_SIZE] = {0};
////    int valread;
////
////    int client_num = -1;
////
////    // Creating socket file descriptor
////    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
////        perror("socket failed");
////        exit(EXIT_FAILURE);
////    }
////
////    set_nonblocking(server_fd);
////
////    // Setting up the address struct
////    address.sin_family = AF_INET;
////    address.sin_addr.s_addr = INADDR_ANY;
////    address.sin_port = htons(PORT);
////
////    // Binding the socket to the port
////    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
////        perror("bind failed");
////        close(server_fd);
////        exit(EXIT_FAILURE);
////    }
////
////    // Listening for incoming connections
////    if (listen(server_fd, 3) < 0) {
////        perror("listen");
////        close(server_fd);
////        exit(EXIT_FAILURE);
////    }
////
////    printf("Server is listening on port %d\n", PORT);
////
////    while (1) {
////        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
////           // perror("accept");
////            continue;
////        }
////        else
////        {
////            client_num++;
////            printf("Get connect from client %d : %s\n",  client_num, inet_ntoa(address.sin_addr));
////            
////            //每来一个连接，就创建一个线程处理
////            
////            // 创建第一个任务（优先级 50，堆栈大小 64KB）
////            // TASK_ID taskId1 = taskSpawn(
////            //     "MyTask1",            // 任务名称
////            //     50,                   // 优先级
////            //     64 * 1024,            // 堆栈大小（64KB）
////            //     (void (*)(void *))myTask, // 函数指针
////            //     (void *)1,            // 传入参数（任务ID为1）
////            //     0,                    // 无特殊选项
////            //     NULL                   // 用户数据
////            // );
////
////            set_nonblocking(new_socket);
////
////            printf("New connection, socket fd is %d, ip is : %s, port : %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
////            while (1)
////            {
////                valread = read(new_socket, buffer, BUFFER_SIZE);
////                if (valread == 0) 
////                {
////                    printf("Host disconnected, socket fd is %d\n", new_socket);
////                    close(new_socket);
////                    break;
////                } 
////                else if (valread < 0) 
////                {
////                    {
////                        //printf("read\n");
////                        //close(new_socket);
////                        //break;
////                        continue;
////                    }
////                } 
////                else 
////                {
////                    //buffer[valread] = '\0';
////                    //printf("Received: %s\n", buffer);
////                    handle_command(new_socket, buffer, valread);
////                }
////            }
////        }
////       
////    }
////
////    close(new_socket);
////    close(server_fd);
////
////    return 0;
////}
//
//// 任务函数：处理客户端连接
//void FixedPortClient(int client_socket) {
//    char buffer[BUFFER_SIZE] = {0};
//    int valread;
//
//    printf("New connection: sock %d\n", client_socket);
//    set_nonblocking(client_socket);
//    
//    //调用AXI_api设置串口相关寄存器
//    unsigned char send_buf[4] = {0x26, 0x00, 0x00, 0x81};
//        int send_len = sizeof(send_buf);
//        
//        if (send(client_socket, send_buf, send_len, 0) != send_len) {
//            perror("send failed");
//            close(client_socket);
//            return;
//        }
//        printf("Sent 0x26000081 to client %d\n", client_socket);
//        
//    while (1) {
//        valread = recv(client_socket, buffer, BUFFER_SIZE, 0);
//        if (valread == 0) {
//            printf("Client disconnected: sock %d\n", client_socket);
//            close(client_socket);
//            return;
//        } else if (valread == ERROR) {
//            if (errno != EAGAIN && errno != EWOULDBLOCK) {
//                perror("recv failed");
//                close(client_socket);
//                return;
//            }
//            taskDelay(10); // 短暂延时避免忙等待
//            continue;
//        }
//
//        handle_command(client_socket, buffer, valread);
//        memset(buffer, 0, BUFFER_SIZE);
//    }
//}
//
//// 主任务：TCP 服务器
//void FixPortTcpTask() {
//    int server_fd, new_socket;
//    struct sockaddr_in server_addr, client_addr;
//    socklen_t addrlen = sizeof(client_addr);
//
//    // 创建 socket
//    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
//        perror("socket failed");
//        return;
//    }
//
//    // 绑定端口
//    server_addr.sin_family = AF_INET;
//    server_addr.sin_addr.s_addr = INADDR_ANY;
//    server_addr.sin_port = htons(PORT);
//    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == ERROR) {
//        perror("bind failed");
//        close(server_fd);
//        return;
//    }
//
//    // 监听端口
//    if (listen(server_fd, MAX_CLIENTSFD) == ERROR) {
//        perror("listen failed");
//        close(server_fd);
//        return;
//    }
//
//    printf("Server listening on port %d\n", PORT);
//    set_nonblocking(server_fd);
//
//    // 主循环：接受连接
//    while (1) {
//        new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
//        if (new_socket == ERROR) {
//            if (errno != EAGAIN && errno != EWOULDBLOCK) {
//                perror("accept failed");
//            }
//            taskDelay(10);
//            continue;
//        }
//
//        // 创建客户端处理任务
//        taskSpawn("clientTask", 100, 0, 20000, 
//                  (FUNCPTR)FixedPortClient, new_socket, 0, 0, 0, 0, 0, 0, 0, 0, 0);
//    }
//
//    close(server_fd);
//}
//
//// 启动服务器任务
//void startPortTcpServer() {
//    taskSpawn("tcpServer", 90, 0, 32768, (FUNCPTR)FixPortTcpTask, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
//}
