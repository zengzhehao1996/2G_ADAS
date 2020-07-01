/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to utilize GPIO on AiDong Linde-PA board
 *
 */

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <device.h>
#include <gpio.h>
#include <uart.h>
#include <misc/util.h>
#include "my_misc.h"

/* boards/${arch}/${board}/board.h */
#include "board.h"
#include "atc.h"

static struct device *gpio_pin_init(const char *port_name, int pin, int dir, int val)
{
	struct device *dev;
	
	dev = device_get_binding(port_name);
	if (!dev) {
		printk("Cannot find %s!\n", port_name);
		return NULL;
	}

	if (gpio_pin_configure(dev, pin, dir)) {
		printk("Configure GPIO %s PIN %d DIRECTION %d failed\n",
			port_name, pin, dir);
		return NULL;
	}
	
	if (dir == GPIO_DIR_IN)
		return dev;
    
	gpio_pin_write(dev, pin, val);
	return dev;
}

#ifdef PWR_HOLD_GPIO_PORT	
/* enable PWR_HOLD, to enable battery during power supply loss */
static inline int enable_pwr_hold(void)
{
	if (gpio_pin_init(PWR_HOLD_GPIO_PORT, PWR_HOLD_GPIO_PIN, GPIO_DIR_OUT, 1)) {
		printk("Set   GPIO pin PWR_HOLD\n");
		return 0;
	} else {
		printk("Set   GPIO pin PWR_HOLD failed\n");
		return -1;
	}
}
#endif

static inline int enable_gsm_gps_module(void)
{
	struct device *dev;
	
	dev = gpio_pin_init(GSM_EN_GPIO_PORT, GSM_EN_GPIO_PIN, GPIO_DIR_OUT, 1);
	if (dev == NULL) {
		printk("Set   GPIO pin GMS_EN failed\n");
		return -1;
	}
	//printk("Set   GPIO pin GMS_EN\n");
#ifdef GSM_OE_GPIO_PORT	
	dev = gpio_pin_init(GSM_OE_GPIO_PORT, GSM_OE_GPIO_PIN, GPIO_DIR_OUT, 1);
	if (dev == NULL) {
		printk("Set   GPIO pin GMS_OE failed\n");
		return -1;
	}
	//printk("Set   GPIO pin GMS_OE\n");
#endif	
	return 0;
}

static inline int enable_gps(void)
{
	if (gpio_pin_init(GNSS_EN_GPIO_PORT, GNSS_EN_GPIO_PIN, GPIO_DIR_OUT, 1)) {
		//printk("Set   GPIO pin GNSS_EN\n");
		return 0;
	} else {
		printk("Set   GPIO pin GNSS_EN failed\n");
		return -1;
	}
}

int enable_gsm_power(void)
{
	struct device *dev;
    
    print_log("power off gsm ..................\n");
    dev = gpio_pin_init(GSM_PWR_GPIO_PORT, GSM_PWR_GPIO_PIN, GPIO_DIR_OUT, 0);
    k_sleep(2000);
	//printk("Set   GPIO pin GSM_POWER\n");
	print_log("power on gsm ..................\n");
	dev = gpio_pin_init(GSM_PWR_GPIO_PORT, GSM_PWR_GPIO_PIN, GPIO_DIR_OUT, 1);
	if (!dev)
		return -1;
#if 0	
	k_sleep(K_SECONDS(1));

	printk("Clear GPIO pin GSM_POWER\n");
	if (gpio_pin_write(dev, GSM_PWR_GPIO_PIN, 0)) {
		printk("Fail to clear GPIO pin GSM_PWR\n");
		return -1;
	}
	k_sleep(K_SECONDS(1));

	printk("Set   GPIO pin GSM_POWER\n");
	if (gpio_pin_write(dev, GSM_PWR_GPIO_PIN, 1)) {
		printk("Fail to set GPIO pin GSM_PWR\n");
		return -1;
	}
	k_sleep(K_SECONDS(1));
#endif
	
	return 0;
}

int disable_gsm_power(void)
{
    struct device *dev;
    dev = gpio_pin_init(GSM_PWR_GPIO_PORT, GSM_PWR_GPIO_PIN, GPIO_DIR_OUT, 0);
    return 0;

}

static int init_gpio_devices(void)
{

	if (enable_gsm_power()) {
		printk("Fail to enable GSM Power, aborted.\n");
		return -1;
	}
	
	if (enable_gsm_gps_module()) {
		printk("Fail to enable GSM/GPS module, aborted.\n");
		return -1;
	}
	
	if (enable_gps()) {
		printk("Fail to enable GPS output, aborted.\n");
		return -1;
	}
	
	return 0;
}

void init_gpio(void)
{
	init_gpio_devices();
}
