/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <misc/printk.h>
#include <gpio.h>
#include <board.h>
#include "my_misc.h"
#include "hw_can.h"
#include "hw_gpio.h"
#include "hw_rs485.h"
#include "hw_uart.h"
#include "uartThread.h"
#include "gpioThread.h"
#include "thread_led.h"

void main(void)
{ 
  int counter=0;


    thread_led_stat_init();
    thread_led_stat_set(LED_STAT_OFF, 1);
    hw_gpio_init();
    
    startRs485Thread();
    startUartThread();
    startCanThread();
    startGpioThread();
    startTestProcessThread();

  while(1){
    ++counter;
    printk("can send %d count.\n", counter);
    //getTestGpiVal();
    k_sleep(20000);
  }
}

#if 0



#endif

