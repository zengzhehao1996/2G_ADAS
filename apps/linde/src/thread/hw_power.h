#ifndef __HW_POWER_H__
#define __HW_POWER_H__
#include <kernel.h>
#include "rtc.h"
typedef struct {
    uint32_t ts;       /* timestamp */
    localRTC_t rtc;
}powerOffFile_t;

bool hwPowerEnable(void);
bool hwPowerCallbackInit(uint8_t canType, uint8_t authType);

#endif  /* end of hw_power_h */
