#ifndef __RTC_TIMESTAMP_H__
#define __RTC_TIMESTAMP_H__
#include <kernel.h>
#include "rtc.h"
#define UTC_BASE_YEAR 1970
#define MONTH_PER_YEAR 12
#define DAY_PER_YEAR 365
#define SEC_PER_DAY 86400
#define SEC_PER_HOUR 3600
#define SEC_PER_MIN 60

uint8_t isLeapYear(uint16_t year);

uint8_t dayOfMonth(uint8_t month, uint16_t year);

uint32_t RTC2TimeStamp(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

localRTC_t * timeStamp2RTC(const uint32_t srctime, localRTC_t *rtc);

#endif