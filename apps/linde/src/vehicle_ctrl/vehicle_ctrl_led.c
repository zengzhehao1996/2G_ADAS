#include "vehicle_ctrl_led.h"
#include "my_misc.h"
#include <gpio.h>
#include <device.h>
#include <board.h>
#include <stdio.h>
#include <stdint.h>
//#define VEHICLE_LED_OPEN_VAL    1
//#define VEHICLE_LED_CLOSE_VAL  0
static struct device *g_vehclCtrlLed;

bool vehclCtrlLedSetup()
{
    g_vehclCtrlLed = device_get_binding(GPIO_VEHCL_CTRL_LED_PORT);
    if (!g_vehclCtrlLed) {
      print_log("Cannot find %p!\n", g_vehclCtrlLed);
      return false;
    }else{
       print_log("g_vehclCtrlLed [%p]\n",g_vehclCtrlLed);
    }
    if (gpio_pin_configure(g_vehclCtrlLed, GPIO_VEHCL_CTRL_LED_PIN, GPIO_DIR_OUT)) {
       print_log("Configure vehclCtrlLedSetup failed");
       return false;
    }
    return true;

}
bool vehclLedOpen()
{
    if(gpio_pin_write(g_vehclCtrlLed, GPIO_VEHCL_CTRL_LED_PIN, VEHICLE_LED_OPEN_VAL))
    {
        return false;
    }
    return true;
}
bool vehclLedClose()
{
    if(gpio_pin_write(g_vehclCtrlLed, GPIO_VEHCL_CTRL_LED_PIN, VEHICLE_LED_CLOSE_VAL))
    {
        return false;
    }
    return true;

}



