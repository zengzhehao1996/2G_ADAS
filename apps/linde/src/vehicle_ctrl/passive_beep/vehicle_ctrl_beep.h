/*passive_beep for v1.4*/
#ifndef __VEHICLE_CTRL_BEEP_H__
#define __VEHICLE_CTRL_BEEP_H__

#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>
#include "passive_beep.h"

#define vehclCtrlBeepSetup() passiveBeepSetup()
#define vehclBeepOpen() passiveBeepOpen()
#define vehclBeepClose() passiveBeepClose()
#define vehclLockBeepHint() passiveBeepOnetime()
#define vehclUnlockBeepHint() passiveBeepOnetime()

#endif
