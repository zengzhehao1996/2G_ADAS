#ifndef __GPIOTHREA_H__
#define __GPIOTHREA_H__
#include <kernel.h>

bool startGpioThread(void);
void stopGpioThread(void);
bool getTestGpiVal();
bool isGpioThreadStart();

#endif
