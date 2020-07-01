/*active_beep for v1.3*/
#ifndef __VEHICLE_CTRL_BEEP_H__
#define __VEHICLE_CTRL_BEEP_H__
#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>
#include "active_beep.h"

#define vehclCtrlBeepSetup() activeBeepSetup()
#define vehclBeepOpen() activeBeepOpen()
#define vehclBeepClose() activeBeepClose()
#define vehclLockBeepHint() NULL
#define vehclUnlockBeepHint() NULL


#endif

