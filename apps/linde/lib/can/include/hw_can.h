#ifndef _CAN_H_
#define _CAN_H_

#include <kernel.h>

#include "my_misc.h"
#include <stm32f4xx.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"

#define MAX_CAN_INIT_NUM  3
#define CAN_RECV_BUF_SIZE 128

BOOL initDeviceCan(uint8_t protoIndex);
BOOL configStandardIdFilter(uint16_t* stdIdArray, uint8_t idCount);
BOOL sendStdCanMessage(uint16_t id, uint8_t* data, uint8_t len);
uint32_t getLastCanAlive (void);
uint8_t getCanRecvCount();
CanRxMsgTypeDef* getCanRecvMessage(uint8_t index);
void resetCanRecvBuff(void);

#endif //_CAN_H_
