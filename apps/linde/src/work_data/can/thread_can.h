#ifndef __THREAD_CAN_H__
#define __THREAD_CAN_H__

#include <kernel.h>
typedef struct 
{
    uint8_t canType;
    uint8_t canPattern;
    uint8_t authType;
    uint16_t interval;
    uint16_t seatTimeOut;
    uint32_t overSpeed;
    uint8_t move_type;          //nocan config
    uint8_t fork_type;          //nocan config
    uint16_t move_threshold;    //nocan config
    uint16_t fork_threshold;    //nocan config
    uint8_t pressEnble;
    uint8_t seatType;
    uint16_t carry_threshold;
    uint16_t overload_threshold;
    uint16_t canOfflineInterval;
}workDataPara_t;

bool startCanThread(workDataPara_t ps);
void stopCanThread(void);
void detect_can_thread(uint32_t ts);

#endif 