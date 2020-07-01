#include "hw_power.h"
#include <board.h>
#include <gpio.h>
#include <device.h>
#include "hw_gpio.h"
#include "my_misc.h"

static uint8_t g_canType;
static uint8_t g_authType;

/* power hold */
static struct device* gpio_power_pin = NULL;

/* power ext callback */
#define DELAY_POWER_OFF_TIME 1000
static struct device*       g_gpio_pwr_ext = NULL;
static struct gpio_callback g_gpio_pwr_ext_callback;
static void hwPowerExtCallback(struct device* port, struct gpio_callback* cb, u32_t pins);
static void hwPowerExtCallbackWork(struct k_work* p);
static void hwPowerExtCallbackTimer(struct k_timer* timer);
K_TIMER_DEFINE(g_extpwr_timer, hwPowerExtCallbackTimer, NULL);


static struct k_work g_extpwr_work;

bool hwPowerDisable(void);

static bool hwPowerHoldInit(void)
{
    gpio_power_pin = hwGpioPinInit(PWR_HOLD_GPIO_PORT, PWR_HOLD_GPIO_PIN, GPIO_DIR_OUT);
    if(!gpio_power_pin)
    {
        printk("GPIO %s PIN %d configure failed\n", PWR_HOLD_GPIO_PORT, PWR_HOLD_GPIO_PIN);
        return false;
    }
    return true;
}

bool hwPowerEnable(void)
{
    if(!gpio_power_pin)
    {
        if(!hwPowerHoldInit())
        {
            return false;
        }
    }
    int ret = gpio_pin_write(gpio_power_pin, PWR_HOLD_GPIO_PIN, 1);
    print_log("Power Enable.\n");
    return (0 == ret);
}

bool hwPowerDisable(void)
{
    if(!gpio_power_pin)
    {
        if(!hwPowerHoldInit())
        {
            return false;
        }
    }
    print_log("Power Disable.\n");
    int ret = gpio_pin_write(gpio_power_pin, PWR_HOLD_GPIO_PIN, 0);
    return (0 == ret);
}

static bool isHwPowerOn(void)
{
    unsigned int pwr;
    gpio_pin_read(g_gpio_pwr_ext, PWR_EXT_GPIO_PIN, &pwr);
    if(0 == pwr)
    {
        return false;
    }
    else
    {
        return true;
    }
}

static void hwPowerExtCallback(struct device* port, struct gpio_callback* cb, u32_t pins)
{
    k_work_submit(&g_extpwr_work); /* No delay operation in interrupt */
}

static void hwPowerExtCallbackWork(struct k_work* p)
{
    if(isHwPowerOn())
    {
        k_timer_stop(&g_extpwr_timer);
        print_log("$$$$$$$$ EXT POWER UP!!! $$$$$$$$\n");
    }
    else
    {
        k_timer_start(&g_extpwr_timer, DELAY_POWER_OFF_TIME, 0);
        print_log("######## EXT POWER DOWN!!! ########\n");
    }
}

static void hwPowerExtCallbackTimer(struct k_timer* timer)
{
    if(isHwPowerOn())
    {
        return;
    }

    //if ext power out
    print_log(">>>>>>>>>>>>>> POWER OFF NOWWWWWWWWW!!!! <<<<<<<<<<<<<<<\n");
    semGivePowerOff();

}



bool hwPowerCallbackInit(uint8_t canType, uint8_t authType)
{
    g_canType = canType;
    g_authType = authType;

    g_gpio_pwr_ext = device_get_binding(PWR_EXT_GPIO_PORT);
    if(!g_gpio_pwr_ext)
    {
        err_log("Init Power EXT callback pin failed.\n");
        return false;
    }
    gpio_pin_configure(g_gpio_pwr_ext, PWR_EXT_GPIO_PIN,
                       GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_INT_DOUBLE_EDGE);
    gpio_init_callback(&g_gpio_pwr_ext_callback, hwPowerExtCallback, BIT(PWR_EXT_GPIO_PIN));
    gpio_add_callback(g_gpio_pwr_ext, &g_gpio_pwr_ext_callback);
    gpio_pin_enable_callback(g_gpio_pwr_ext, PWR_EXT_GPIO_PIN);

    k_work_init(&g_extpwr_work, hwPowerExtCallbackWork);

    return true;
}