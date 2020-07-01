#ifndef __HW_RS485_H_
#define __HW_RS485_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdbool.h>
#include <stdint.h>
#include <uart.h>
#include <kernel.h>

bool hw_rs485_init();

void hw_rs485_send(uint8_t *data,int len);
int  hw_rs485_read(uint8_t * out_data,int size);
int hw_rs485_availabe_num();
bool hw_rs485_unint();

#ifdef __cplusplus
}
#endif
#endif //__HW_RS485_H_

