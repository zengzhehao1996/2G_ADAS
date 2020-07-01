/*
 * thread_led.h
 *
 *  Created on: Nov 23, 2017
 *      Author: ls
 */

#ifndef SAMPLES_LINDE_V1_0_SRC_THREAD_LED_CPP_
#define SAMPLES_LINDE_V1_0_SRC_THREAD_LED_CPP_

#ifdef __cplusplus
extern "C"{
#endif

#include  "kernel.h"

#define  LED_STAT_OFF               0
#define  LED_STAT_ON               (1<<0)
#define  LED_FOTA_SIGNAL           (1<<1)
#define  LED_STAT_NO_CAN_SIGNAL    (1<<2)
#define  LED_STAT_NO_GPS_SIGNAL    (1<<3)
#define  LED_STAT_NO_COMM_SIGNAL   (1<<4)
#define  LED_STAT_POWER_EXT        (1<<5)
#define  LED_RESET                 (1<<6)

//void thread_led_stat_set(char cmdid);
void thread_led_stat_set(char comid, char stat);
void thread_led_stat_init(void);
void thread_led_stop(void);
s64_t thread_led_alive_get_last_time_ms(void);
bool ledThreadReay();

#ifdef __cplusplus
}
#endif


#endif /* SAMPLES_LINDE_V1_0_SRC_THREAD_LED_CPP_ */
