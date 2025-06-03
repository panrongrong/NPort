#ifndef SERIAL_BRIDGE_H_
#define SERIAL_BRIDGE_H_

#include "common.h"

/* LED control */
void txled(int i, int action);
void rxled(int i, int action);
void Portled(int i, int action);

/* UART functions */
void uart_ring_buffer_enqueue(ring_buffer_t *buffer, const char *data, ring_buffer_size_t size);
ring_buffer_size_t uart_ring_buffer_dequeue(ring_buffer_t *buffer, char *data, ring_buffer_size_t len);
void update_led_state(uart_led_stat_t *stat, int is_tx);

/* UART task functions */
void multi_uart_tx_loop(int unused);
void multi_uart_rx_loop(int unused);
void multi_uart_forward_loop(int unused);
int calc_poll_delay_ticks(unsigned int baud_rate);

#endif /* SERIAL_BRIDGE_H_ */
