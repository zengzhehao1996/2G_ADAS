#include "hw_gpio.h"
#include <zephyr.h>
#include <misc/printk.h>

#include <device.h>
#include <gpio.h>
#include <misc/util.h>

#define print_log(fmt,arg...) {printk("[%s,%d] ",__FILE__,__LINE__);printk(fmt,##arg);}

// #define CFG_INIT_GPIO_PORT "GPIOB"
// #define CFG_INIT_GPIO_PIN  9
// struct device *hw_gpio_cfg;
#define PWR_HOLD_GPIO_PORT "GPIOC"
#define PWR_HOLD_GPIO_PIN 0
#define LED_GPIO_PORT PWR_HOLD_GPIO_PORT
#define LED_GPIO_PIN 1

struct device *gpio_pin_init(const char *port_name, int pin, int dir)
{
  struct device *dev;

  dev = device_get_binding(port_name);
  if(!dev){
    print_log("Cannot find %s!\n", port_name);
	return NULL;
  }
  if(gpio_pin_configure(dev, pin, dir) == 0){
    return dev;
  }else{
    return NULL;
  }

}

void hw_gpio_init(void)
{
  struct device *gpio_pwr_hold;
  gpio_pwr_hold = gpio_pin_init(PWR_HOLD_GPIO_PORT, PWR_HOLD_GPIO_PIN, GPIO_DIR_OUT);
	if (!gpio_pwr_hold) {
		printk("GPIO %s PIN %d configure failed\n", PWR_HOLD_GPIO_PORT, PWR_HOLD_GPIO_PIN);
		return;
	}
  /* enable PWR_HOLD, to enable battery during power supply loss */
	gpio_pin_write(gpio_pwr_hold, PWR_HOLD_GPIO_PIN, 1);

  struct device *led_stat;
  led_stat = gpio_pin_init(LED_GPIO_PORT,LED_GPIO_PIN,GPIO_DIR_OUT);
  if(!led_stat){
    printk("GPIO %s Pin %d configure failed\n",LED_GPIO_PORT,LED_GPIO_PIN);
  }
  gpio_pin_write(led_stat, LED_GPIO_PIN, 0);

}

// int hw_seat_stat(void)
// {
//   int val=0;
//   gpio_pin_read(hw_gpio_cfg, CFG_INIT_GPIO_PIN, &val);
//   return val;
// }
