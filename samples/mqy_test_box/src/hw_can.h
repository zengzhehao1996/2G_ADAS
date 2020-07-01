#ifndef _HW_CAN_H_
#define _HW_CAN_H_

#include <kernel.h>

#include "my_misc.h"
#include <stm32f4xx.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"


BOOL hw_can_init(BOOL isNormal);
BOOL hw_can_isNormal(void);
BOOL hw_can_write(uint32_t id,uint8_t* data,int len);
s64_t hw_can_last_rx_ts(void);

void hw_can_rx_list_reset(void);
int hw_can_rx_list_length(void);
CanRxMsgTypeDef * hw_can_rx_list_message(int index);


#endif //_HW_CAN_H_
