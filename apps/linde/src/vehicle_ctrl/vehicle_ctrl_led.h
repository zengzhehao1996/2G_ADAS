#ifndef __VEHICLE_CTRL_LED_H__
#define __VEHICLE_CTRL_LED_H__

#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>

bool vehclCtrlLedSetup();
bool vehclLedOpen();
bool vehclLedClose();

#endif
