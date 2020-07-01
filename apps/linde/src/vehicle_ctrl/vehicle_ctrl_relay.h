#ifndef __VEHICLE_CTRL_RELAY_H__
#define __VEHICLE_CTRL_RELAY_H__

#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>

bool vehclCtrlrelaySetup();
bool vehclOpen();
bool vehclLock();
/*
*return: -1 get state failed; VEHICLE_LOCK_VAL is lock ;VEHICLE_UNLOCK_VAL is open
*/
char vehclRelayState();

#endif
