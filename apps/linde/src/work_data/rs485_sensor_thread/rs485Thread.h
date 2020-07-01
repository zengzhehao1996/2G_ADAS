#ifndef __RS485THREA_H__
#define __RS485THREAD_H__
#include "tray_pressure.h"
#include <kernel.h>

typedef struct
{
    uint16_t carryThreshold;
    uint16_t overLoadThreshold;
    int8_t mode;//run mode :aidong factory test, normal mode
}rs485para_t;
typedef struct 
{
    char rs485Stat;
    uint16_t  timeout;
    uint16_t contrlGpoStat;
    uint16_t reserved;
}readTestbox;

typedef struct
{
    uint16_t testResult;
    uint16_t reserved;
    bool sendFlag;
}setTestbox;

bool startRs485Thread(rs485para_t *ps);
void stopRs485Thread(void);
bool getTestBoxData(readTestbox* readBox);
bool setTestBox_(setTestbox cmd);
bool setTBsucess();
bool rs485TMready();
#endif
