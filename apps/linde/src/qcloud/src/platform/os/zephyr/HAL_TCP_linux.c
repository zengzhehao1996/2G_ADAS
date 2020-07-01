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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include "atcip.h"

#include "qcloud_iot_import.h"
#include "qcloud_iot_export_log.h"
#include "qcloud_iot_export_error.h"

#include "my_misc.h"

static uint64_t _linux_get_time_ms(void)
{
    struct timeval tv = { 0 };
    uint64_t time_ms;

    gettimeofday(&tv, NULL);

    time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return time_ms;
}

static uint64_t _linux_time_left(uint64_t t_end, uint64_t t_now)
{
    uint64_t t_left;

    if (t_end > t_now) {
        t_left = t_end - t_now;
    } else {
        t_left = 0;
    }

    return t_left;
}

#define CONNECT_TIMEOUT_MS	30000
uintptr_t HAL_TCP_Connect(const char *host, uint16_t port)
{
    int ret;
    print_log("call hal tcp connect....................................\n");
    ret = atcip_open(host, port, CONNECT_TIMEOUT_MS);

    return (uintptr_t)ret;
}
#undef CONNECT_TIMEOUT_MS

int HAL_TCP_Disconnect(uintptr_t fd)
{
    atcip_close();
    return 0;
}


int HAL_TCP_Write(uintptr_t fd, const unsigned char *buf, uint32_t len, uint32_t timeout_ms, size_t *written_len)
{
    int ret;

    ret = atcip_send(buf, len);

    *written_len = (size_t)ret;

    return ret > 0 ? QCLOUD_ERR_SUCCESS : ret;
}


int HAL_TCP_Read(uintptr_t fd, unsigned char *buf, uint32_t len, uint32_t timeout_ms, size_t *read_len)
{
    int ret;

    ret = atcip_recv(buf, len, timeout_ms);
    
    *read_len = ret;

    return (0 != ret) ? 0 : QCLOUD_ERR_MQTT_NOTHING_TO_READ;
}
