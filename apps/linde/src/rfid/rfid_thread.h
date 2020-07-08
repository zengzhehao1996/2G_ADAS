#ifndef __RFID_THREAD_H__
#define __RFID_THREAD_H__

#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>
bool rfidThreadStart(void);
void rfidThreadStop(void);
s64_t rfidThreadRunLastTime();
void debugRfidErrFlagSet(bool flag);
uint32_t RFID_get(void);


#endif
