#include "my_misc.h"
#include "hw_gpio.h"

struct device *hwGpioPinInit(const char *portName, int pin, int dir)
{
  struct device *dev;

  dev = device_get_binding(portName);
  if (!dev) 
  {
    print_log("Cannot binding [%s] port!\n", portName);
    return NULL;
  }
  if (gpio_pin_configure(dev, pin, dir) == 0)
  {
    return dev;
  }
  else
  {
    return NULL;
  }
}

