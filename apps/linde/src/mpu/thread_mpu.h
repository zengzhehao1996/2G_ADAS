#ifndef THREAD_MPU_H
#define THREAD_MPU_H

#include "my_misc.h"
#include "config.h"
#include <kernel.h>


bool threadMpuStart(speedLimitConfig_t* ps);
void threadMpuStop(void);
uint8_t getMpuStat();
s64_t thread_mpu_main_loop_run_alive_get_last_time_ms(void);
void detect_thread_mpu(uint32_t current_time);

bool getMpuConfig(speedLimitConfig_t* ps);
bool setMpuConfig(speedLimitConfig_t* ps);

#endif //THREAD_MPU_H

