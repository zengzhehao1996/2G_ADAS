#ifndef __ACTIVE_BEEP_H__
#define __ACTIVE_BEEP_H__
#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>

bool activeBeepSetup();
bool activeBeepOpen();
bool activeBeepClose();

#endif

