#ifndef __HW_UART_H__
#define __HW_UART_H__

#include <stdint.h>

enum hw_uart_num{
  HW_UART6 = 6
};

int hw_uart_init();
int hw_uart_send_bytes(enum hw_uart_num num, const uint8_t *data, int len);
int hw_uart_read_bytes(enum hw_uart_num num, uint8_t *data, int size);
int hw_uart_rx_buff_num(void);

#endif