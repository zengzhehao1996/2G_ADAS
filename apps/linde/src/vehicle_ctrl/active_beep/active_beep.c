#include "active_beep.h"
#include "my_misc.h"
#include <gpio.h>
#include <device.h>
#include <board.h>
#include <stdio.h>
#include <stdint.h>
#define ACTIVE_BEEP_OPEN_VAL    1
#define VEHICLE_BEEP_CLOSE_VAL  0
static struct device *g_activeBeep;
bool activeBeepSetup()
{
    g_activeBeep = device_get_binding(GPIO_VEHCL_CTRL_BEEP_PORT);
    if (!g_activeBeep) {
      print_log("Cannot find %s!\n", g_activeBeep);
      return false;
    }else{
       print_log("g_activeBeep [%p]\n",g_activeBeep);
    }
    if (gpio_pin_configure(g_activeBeep, GPIO_VEHCL_CTRL_BEEP_PIN, GPIO_DIR_OUT)) {
       print_log("Configure activeBeepSetup faiBeep");
       return false;
    }
    return true;

}
bool activeBeepOpen()
{
    if(gpio_pin_write(g_activeBeep, GPIO_VEHCL_CTRL_BEEP_PIN, ACTIVE_BEEP_OPEN_VAL))
    {
        return false;
    }
    return true;
}
bool activeBeepClose()
{
    if(gpio_pin_write(g_activeBeep, GPIO_VEHCL_CTRL_BEEP_PIN, VEHICLE_BEEP_CLOSE_VAL))
    {
        return false;
    }
    return true;

}


