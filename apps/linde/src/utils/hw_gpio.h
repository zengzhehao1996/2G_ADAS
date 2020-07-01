#ifndef __HW_GPIO_H__
#define __HW_GPIO_H__

#include <gpio.h>
#include <device.h>

struct device *hwGpioPinInit(const char *portName, int pin, int dir);

#endif
