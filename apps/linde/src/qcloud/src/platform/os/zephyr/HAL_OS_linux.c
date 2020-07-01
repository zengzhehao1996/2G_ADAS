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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "zephyr.h"

#include <sys/types.h>
#include "qcloud_iot_import.h"

void *HAL_MutexCreate(void)
{
    struct k_mutex *mutex = (struct k_mutex *)HAL_Malloc(sizeof(struct k_mutex));
    if (NULL == mutex) {
        printk("create mutex fail, size:%d\n",sizeof(struct k_mutex));
        return NULL;
    }

    k_mutex_init((struct k_mutex *)mutex);

    return mutex;
}

void HAL_MutexDestroy(_IN_ void *mutex)
{
    HAL_Free(mutex);
}

void HAL_MutexLock(_IN_ void *mutex)
{
    int err_num;
    if (0 != (err_num = k_mutex_lock(mutex, K_NO_WAIT))) {
        printk("lock mutex failed");
    }
}

void HAL_MutexUnlock(_IN_ void *mutex)
{
    k_mutex_unlock(mutex);
}

void *HAL_Malloc(_IN_ uint32_t size)
{
    //printk("HAL_Malloc size: %d\n", size);
    return k_malloc(size);
}

void HAL_Free(_IN_ void *ptr)
{
    k_free(ptr);
}


void HAL_Printf(_IN_ const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    //fflush(stdout);
}

int HAL_Snprintf(_IN_ char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

int HAL_Vsnprintf(_IN_ char *str, _IN_ const int len, _IN_ const char *format, va_list ap)
{
    return vsnprintf(str, len, format, ap);
}

uint32_t HAL_UptimeMs(void)
{
    return k_uptime_get_32();
}

void HAL_SleepMs(_IN_ uint32_t ms)
{
    k_sleep(ms);
}
