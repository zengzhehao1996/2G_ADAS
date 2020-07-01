#ifndef __VEHICLE_CTRL_THREAD_H__
#define __VEHICLE_CTRL_THREAD_H__

#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>

enum
{
    MODE_FACTORY_TEST = 0,
    MODE_OFFICAL = 1,
    MODE_ABNORMAL = 2
};

enum
{
    UNLOCK_NORMAL = 0,
    UNLOCK_ABNORMAL = 1
};
typedef struct 
{
    char mode;
}vehPara_t;
bool vehclCtrlThreadStart(vehPara_t* para);
void vehclThreadStop(void);
s64_t vehclThreadRunLastTime(void);
uint32_t vehicleGetCurrRfid();
void readPowerOffFile(void);



#endif
