#ifndef __FOTA_UPGRADE_THREAD_H__
#define __FOTA_UPGRADE_THREAD_H__
#include <kernel.h>

void checkFOTA(void);

void doFotaStart(uint8_t* payload_data, uint32_t payload_len);
void pushFotaBytes(uint8_t* payload_data, uint32_t payload_len);
void doFotaSolidifyFirm(void);
void doFotaStop(void);

#endif