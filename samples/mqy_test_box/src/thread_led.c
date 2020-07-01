/*
 * thread_led.c
 *
 *  Created on: Nov 23, 2017
 *      Author: lf
 */
#include "my_misc.h"
#include "hw_led.h"
#include "thread_led.h"

#include  "kernel.h"

#define THREAD_LED_STAT_SIZE 1024
K_THREAD_STACK_DEFINE(thread_led_stack, THREAD_LED_STAT_SIZE);
static struct k_thread  thread_led_stat;

#define LOW 0
#define HIGH 1

unsigned char led_stat = 0;
int task_led_up_interval = 0;
int task_led_down_interval = 0;
static s64_t thread_led_alive_last_time = 0;
static bool ledThreadReayFlag = false;

void task_led_stat_entry(void* ptr){
  static int last_time = 0;
  static char last_state = LOW;
  while(1){
    int current_time = k_uptime_get();
    switch (last_state)
    {
    case HIGH:
      if (current_time - last_time > task_led_up_interval) {
        last_state = LOW;
        led_off();
        last_time = current_time;
      }
    break;
    case LOW:
      if (current_time - last_time > task_led_down_interval) {
        last_state = HIGH;
        led_on();
        last_time = current_time;
      }
    }
    k_sleep(1);
  }
}

static void set_led_interval(int up, int down){
  task_led_up_interval = up;
  task_led_down_interval = down;
}

static void thread_led_entry(void* ptr){
  const int blink = 100;
  const int can_blink = 1000 - blink;
  const int gps_blink = 1000 - blink * 5;
  const int comm_blink = 1000 - blink * 3;
  const int bat_blink = 1000;
  const int stat_hold = 10000;
  const int stat_high = 1000000;
  const int pwr_ext_blink = 2000 - bat_blink;
  const int stat_switch = 2;
  ledThreadReayFlag = true;
  thread_led_alive_last_time = k_uptime_get();
  while (1) {
    if (0 == led_stat) {
      led_off();
      k_sleep(blink);
      goto THREAD_LED_DELAY;
    }

    if ((led_stat & LED_STAT_ON)  && 
        !(led_stat & (LED_STAT_NO_CAN_SIGNAL | 
        LED_STAT_NO_COMM_SIGNAL | LED_STAT_NO_GPS_SIGNAL | LED_STAT_POWER_EXT | LED_FOTA_SIGNAL))) {
      led_on();
    }

    if(led_stat & LED_FOTA_SIGNAL){
      for (int i = 0; i < stat_switch; ++i) {
        led_on();
        k_sleep(blink);
        led_off();
        k_sleep(blink);
      }
      goto THREAD_LED_DELAY;
    }

    if (led_stat & LED_STAT_NO_CAN_SIGNAL) {
      for (int i = 0; i < stat_switch; ++i) {
        led_on();
        k_sleep(blink);
        led_off();
        k_sleep(can_blink);
      }
    }

    if (led_stat & LED_STAT_NO_COMM_SIGNAL) {
      for (int i = 0; i < stat_switch; ++i) {
        led_on();
        k_sleep(blink);
        led_off();
        k_sleep(blink);
        led_on();
        k_sleep(blink);
        led_off();
        k_sleep(comm_blink);
      }
    }

    if (led_stat & LED_STAT_NO_GPS_SIGNAL) {
      for (int i = 0; i < stat_switch; i++) {
        led_on();
        k_sleep(blink);
        led_off();
        k_sleep(blink);
        led_on();
        k_sleep(blink);
        led_off();
        k_sleep(blink);
        led_on();
        k_sleep(blink);
        led_off();
        k_sleep(gps_blink);
      }
    }

    if (led_stat & LED_STAT_POWER_EXT) {
      for (int i = 0; i < stat_switch; ++i) {
        led_on();
        k_sleep(bat_blink);
        led_off();
        k_sleep(pwr_ext_blink);
      }
    }

   if (led_stat & LED_STAT_POWER_EXT) {
	 for (int i = 0; i < stat_switch; ++i) {
	   led_on();
	   k_sleep(2*blink);
	   led_off();
	   k_sleep(2*blink);
	}
   }

THREAD_LED_DELAY: 
   k_sleep(10);
   thread_led_alive_last_time = k_uptime_get();
  }
}

void thread_led_stat_set(char cmdid, char stat){
//print_log("KKKKKKKKKKKKKKKKKKKKKKKKKK led stat set cmdid = %d,stat = %d\n",cmdid,stat);
  switch(cmdid){
    case LED_STAT_OFF: 
      led_stat = 0;
      break;
    case LED_STAT_ON:
      if(stat){
        led_stat |= LED_STAT_ON;
      }else{
        led_stat &= ~LED_STAT_ON;
      }
     break;
    case LED_FOTA_SIGNAL:
      if(stat){
        led_stat |= LED_FOTA_SIGNAL;
            }else{
        led_stat &= ~LED_FOTA_SIGNAL;
      }
      break;
    case LED_STAT_NO_CAN_SIGNAL:
      if(stat){
        led_stat |= LED_STAT_NO_CAN_SIGNAL;
      }else{
        led_stat &= ~LED_STAT_NO_CAN_SIGNAL;
      }
      break;
    case LED_STAT_NO_GPS_SIGNAL:
      if(stat){
        led_stat |= LED_STAT_NO_GPS_SIGNAL;
            }else{
        led_stat &= ~LED_STAT_NO_GPS_SIGNAL;
      }
      break;
    case LED_STAT_NO_COMM_SIGNAL:
      if(stat){
        led_stat |= LED_STAT_NO_COMM_SIGNAL;
            }else{
        led_stat &= ~LED_STAT_NO_COMM_SIGNAL;
      }
      break;
  case LED_STAT_POWER_EXT:
    if(stat){
      led_stat |= LED_STAT_POWER_EXT; 
    }else{
      led_stat &= ~LED_STAT_POWER_EXT; 
    }
  case LED_RESET:
	if(stat){
      led_stat |= LED_RESET; 
    }else{
      led_stat &= ~LED_RESET; 
    }	

    default:
      break;
  }
}

static k_tid_t g_thread_led_id;
void thread_led_stat_init(void){
 
  led_init();
  
  g_thread_led_id = k_thread_create(&thread_led_stat, thread_led_stack,
                                THREAD_LED_STAT_SIZE,
                                (k_thread_entry_t) thread_led_entry,
                                NULL,
                                NULL, NULL, K_PRIO_COOP(5), 0, 0);
  
  if (g_thread_led_id) {
    print_log("Create LED THREAD Id:[ %p ]; Stack:[ %p ]; Size:[ %d ]\n", g_thread_led_id,thread_led_stack,
                                THREAD_LED_STAT_SIZE);
  } else {
    print_log("create led thread faild\n");
  }

}

void thread_led_stop(void)
{
  if(0!=g_thread_led_id){
    k_thread_abort(g_thread_led_id);
    g_thread_led_id=0;
  ledThreadReayFlag = false;
  }
}

s64_t thread_led_alive_get_last_time_ms(void){
  return thread_led_alive_last_time;
}

bool ledThreadReay()
{
    return ledThreadReayFlag;
}

