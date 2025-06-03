#ifndef NET_BRIDGE_H_
#define NET_BRIDGE_H_

#include "common.h"

/* Network functions */
int create_tcp_server_socket(int *sock_fd);
int bind_tcp_server_socket(int sock_fd, int port);
int listen_tcp_server_socket(int sock_fd);
void set_state(uint8_t *var, sock_state_enum new_state, int idx, const char *side, uint16_t port);

/* Server tasks */
void multi_tcp_cmd_servers_loop(int unused);
void multi_tcp_data_servers_loop(int unused);
void check_tcp_cmd_timeouts(void);
void check_tcp_data_timeouts(void);
void heartbeat_send_task(void);

/* Command handling */
void uart_info_send(uint8_t i);
void hear_send(uint8_t i);
void handle_command(UART_Config_Params *uart, int client_fd, char *cmd_buf, int cmd_len, int port_idx);
void usart_report_hearbeat(int client_fd, void *data, int len);

#endif /* NET_BRIDGE_H_ */
