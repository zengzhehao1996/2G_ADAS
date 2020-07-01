#include "vehicle_ctrl_relay.h"
#include "my_misc.h"
#include <gpio.h>
#include <device.h>
#include <board.h>
#include <stdio.h>
#include <stdint.h>

#define VEHICLE_LOCK_VAL    1
#define VEHICLE_UNLOCK_VAL  0
static struct device *g_vehclCtrlRelay;
bool vehclCtrlrelaySetup()
{
    g_vehclCtrlRelay = device_get_binding(GPIO_VEHCL_CTRL_DELAY_PORT);
    if (!g_vehclCtrlRelay) {
      print_log("Cannot find %p!\n", g_vehclCtrlRelay);
      return false;
    }else{
       print_log("g_vehclCtrlRelay [%p]\n",g_vehclCtrlRelay);
    }
    if (gpio_pin_configure(g_vehclCtrlRelay, GOIO_VEHCL_CTRL_DALAY_PIN, GPIO_DIR_OUT)) {
       print_log("Configure g_vehclCtrlRelay failed");
       return false;
    }
    return true;

}
bool vehclOpen()
{
    if(gpio_pin_write(g_vehclCtrlRelay, GOIO_VEHCL_CTRL_DALAY_PIN, VEHICLE_UNLOCK_VAL))
    {
        return false;
    }
    return true;
}
bool vehclLock()
{
    if(gpio_pin_write(g_vehclCtrlRelay, GOIO_VEHCL_CTRL_DALAY_PIN, VEHICLE_LOCK_VAL))
    {
        return false;
    }
    return true;
}
char vehclRelayState()
{
    char pinValue = 0;
    if(gpio_pin_read(g_vehclCtrlRelay,GOIO_VEHCL_CTRL_DALAY_PIN,&pinValue))
    {
        return -1;
    }
    return pinValue;
}

