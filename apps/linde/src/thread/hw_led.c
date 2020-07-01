/*
 * hw_led.c
 *
 *  Created on: Nov 22, 2017
 *      Author: root
 */

#include "hw_led.h"
#include "my_misc.h"
#include <gpio.h>
#include <device.h>
#include "hw_gpio.h"

/* Output - Status LED */
#if defined(CONFIG_BOARD_AIDONG_LINDE429V13)
#define LED_STAT_GPIO_PORT	"GPIOC"
#define LED_STAT_GPIO_PIN	1
#elif defined(CONFIG_BOARD_AIDONG_LINDE429V14)
#define LED_STAT_GPIO_PORT	"GPIOI"
#define LED_STAT_GPIO_PIN	4
#endif


static struct device *gpio_led_stat;

void led_on(void){
  gpio_pin_write(gpio_led_stat, LED_STAT_GPIO_PIN, 0);
  //print_log("Write GPIO pin LED_STAT value %d\n", 1);
}
void led_off(void){
  gpio_pin_write(gpio_led_stat, LED_STAT_GPIO_PIN, 1);
  //print_log("Write GPIO pin LED_STAT value %d\n", 0);
}
void led_init(void){
  gpio_led_stat = hwGpioPinInit(LED_STAT_GPIO_PORT, LED_STAT_GPIO_PIN, GPIO_DIR_OUT);
  	if (!gpio_led_stat) {
  		printk("GPIO %s PIN %d configure failed\n", LED_STAT_GPIO_PORT, LED_STAT_GPIO_PIN);
  		return;
  	}
  	gpio_pin_write(gpio_led_stat, LED_STAT_GPIO_PIN, 0);
}
