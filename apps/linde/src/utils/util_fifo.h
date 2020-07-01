#ifndef __UTIL_FIFO_H__
#define __UTIL_FIFO_H__
#include <stdbool.h>
#include <kernel.h>
#include <stdint.h>
typedef struct myFifo_s
{
    struct k_fifo fifoHandle;
    struct k_sem semHandle;
    struct k_mutex fifoMux;
    uint16_t elementSize;   /*need usr init,fifo size*/
    uint8_t semStart;/*need usr init*/
    uint8_t semEnd;/*need usr init*/
}myFifo_t;

bool myFifoInit(myFifo_t *ptr);
bool myFifoSend(myFifo_t *handlePtr,char *msgPtr);
bool myFifoRcv(myFifo_t *handlePtr, char *msgPtr);

#endif
