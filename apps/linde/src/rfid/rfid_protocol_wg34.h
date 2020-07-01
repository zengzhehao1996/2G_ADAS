#ifndef __RFID_PROTOCOL_WG34_H__
#define __RFID_PROTOCOL_WG34_H__

#include <kernel.h>
#include <stdbool.h>

char rfidWg34Setup();
char rfidWg34UnInit();
unsigned int rfidWg34GetSerilId();
void rfidWg34Reset();
char rfidWg34RcvBitCnt();


#endif