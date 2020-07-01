/*
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif
    
#include "time.h"
#include "zephyr.h"
    
#include "qcloud_iot_import.h"

static char now_time_str[20] = {0};
    
char HAL_Timer_expired(Timer *timer) {
    uint32_t now = k_uptime_get_32();
    uint32_t end = timer->end_time.tv_sec*1000 + timer->end_time.tv_usec/1000;
    return (end<=now);
}

void HAL_Timer_countdown_ms(Timer *timer, unsigned int timeout_ms) {
    uint32_t now = k_uptime_get_32();
    //printk("now: %d\n", now);
    now = now + timeout_ms;
    timer->end_time.tv_sec = now/1000;
    timer->end_time.tv_usec = (now%1000)*1000;
    //printk("end: %d, %d\n", timer->end_time.tv_sec, timer->end_time.tv_usec);
    
}

void HAL_Timer_countdown(Timer *timer, unsigned int timeout) {
    uint32_t now = k_uptime_get_32();
    timer->end_time.tv_sec = now/1000+timeout;
    timer->end_time.tv_usec = (now%1000)*1000;
}

int HAL_Timer_remain(Timer *timer) {
    uint32_t now = k_uptime_get_32();
    uint32_t end = timer->end_time.tv_sec*1000 + timer->end_time.tv_usec/1000;
    return (end<=now) ? 0 : (end-now);
}

void HAL_Timer_init(Timer *timer) {
    timer->end_time = (struct timeval) {0, 0};
}

char* HAL_Timer_current(void) {
    uint32_t now = k_uptime_get_32();
    sprintf(now_time_str, "%u", now);
	return now_time_str;
}

long HAL_Timer_current_sec(void) {
	uint32_t now = k_uptime_get_32();
    return now/1000;
}
    
#ifdef __cplusplus
}
#endif
