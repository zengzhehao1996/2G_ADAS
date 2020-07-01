#include "gpioThread.h"
#include "my_misc.h"
#include <gpio.h>
#include <board.h>
#define GPIO_THREAD_STACK_SIZE 1024
K_THREAD_STACK_DEFINE(g_gpioThreadStack, GPIO_THREAD_STACK_SIZE);
static struct k_thread g_gpioThread;
static k_tid_t         g_gpioThreadId;

#define VEHICLE_LOCK_VAL    1
#define VEHICLE_UNLOCK_VAL  0
static struct device *g_vehclCtrlRelay;
static bool gpioThreadStartFlag = false;

static bool vehclCtrlrelaySetup();
static bool vehclOpen();
static bool vehclLock();
static void gpioThreadRun(void* p);
static bool testGpiSetup();


#define GPIO_RFID_D0      "GPIOE"
#define GOIO_RFID_D0_PIN       13
static struct device *g_gpio_RFID_D0; 


bool startGpioThread(void)
{
    bool ret;

    g_gpioThreadId =
        k_thread_create(&g_gpioThread, g_gpioThreadStack, GPIO_THREAD_STACK_SIZE,
                        (k_thread_entry_t)gpioThreadRun, NULL, NULL, NULL, K_PRIO_COOP(5), 0, 0);
    if(g_gpioThreadId != 0)
    {
        ret = true;
        print_log("Create gpio THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %p ]\n", g_gpioThreadId,
                  g_gpioThreadStack, GPIO_THREAD_STACK_SIZE);
    }
    else
    {
        ret = false;
        err_log("Create gpio thread Failed.\n\n");
    }

    return ret;
}
void stopGpioThread(void)
{
    if(0 != g_gpioThreadId)
    {
        
        k_thread_abort(g_gpioThreadId);
        g_gpioThreadId = 0;
        gpioThreadStartFlag = false;
        print_log("stop gpio Thread  success !\n");
    }
}
static void gpioThreadRun(void* p)
{
    //1.init gpio
    if(!vehclCtrlrelaySetup())
    {
        err_log(" vehicle relay setup failed \n");
        return false;
    }

    //2.init test gpi
    if(!testGpiSetup())
    {
        err_log(" testGpiSetup failed \n");
        return false;
    }
    gpioThreadStartFlag = true;
    while(1)
    {
        static uint16_t delayTime = 2 * 1000; 
        vehclOpen();
        k_sleep(delayTime);
        vehclLock();
        k_sleep(delayTime);
    }
}


static bool vehclCtrlrelaySetup()
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
static bool vehclOpen()
{
    if(gpio_pin_write(g_vehclCtrlRelay, GOIO_VEHCL_CTRL_DALAY_PIN, VEHICLE_UNLOCK_VAL))
    {
        return false;
    }
    return true;
}
static bool vehclLock()
{
    if(gpio_pin_write(g_vehclCtrlRelay, GOIO_VEHCL_CTRL_DALAY_PIN, VEHICLE_LOCK_VAL))
    {
        return false;
    }
    return true;
}

static bool testGpiSetup()
{

    g_gpio_RFID_D0 = device_get_binding(GPIO_RFID_D0);
    if (!g_gpio_RFID_D0) 
    {
      print_log("Cannot find %p!\n", g_gpio_RFID_D0);
      return false;
    }
    else
    {
       print_log("g_gpio_RFID_D0 [%p]\n",g_gpio_RFID_D0);
    }
#if 1
    if (gpio_pin_configure(g_gpio_RFID_D0, GOIO_RFID_D0_PIN, GPIO_DIR_IN)) 
    {
       print_log("Configure g_gpio_RFID_D0 failed");
       return false;
    }
#endif


    return true;
}

bool getTestGpiVal()
{
#if 1
    int32_t val = 0;
    //k_sleep(100);
    gpio_pin_read(g_gpio_RFID_D0,GOIO_RFID_D0_PIN, &val);
   // print_log(" test gpi val[%d]\n",val);
    //k_sleep(100);
    return (val != 0);
    #endif
}
bool isGpioThreadStart()
{
    return gpioThreadStartFlag;
}

