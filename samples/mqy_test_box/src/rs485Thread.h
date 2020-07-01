#ifndef __RS485THREA_H__
#define __RS485THREAD_H__
#include <kernel.h>

#define USARTOK 0
#define USARTREADOVER -1
#define USARTWRITEOVER -2
#define USARTCATOVER -3
#define UART_ENTER_ERR -4

#define RS485_FIFO_RET int
bool startRs485Thread(void);
void stopRs485Thread(void);
void print_buff(uint8_t* buff,int len);
bool isRs485ThreradStart();






#endif
