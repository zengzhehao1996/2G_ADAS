#ifndef __PASSIVE_BEEP_H__
#define __PASSIVE_BEEP_H__
#include <stdint.h>
#include <stdbool.h>
#include <kernel.h>

bool passiveBeepSetup();
bool passiveBeepOpen();
bool passiveBeepClose();
void passiveBeepOnetime();

#endif

