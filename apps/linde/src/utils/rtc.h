#ifndef __RTC_H__
#define __RTC_H__

#include "my_misc.h"
#include "smart_link_version.h"
#include <kernel.h>

#pragma pack(1)
typedef struct
{
  uint8_t year;     /*!< Specifies the RTC Date Year.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 99 */
  uint8_t month;    /*!< Specifies the RTC Date Month (in BCD format).
                         This parameter can be a value of @ref RTC_Month_Date_Definitions */
  uint8_t date;     /*!< Specifies the RTC Date.
                         This parameter must be a number between Min_Data = 1 and Max_Data = 31 */
  uint8_t weekDay;  /*!< Specifies the RTC Date WeekDay.
                         This parameter can be a value of @ref RTC_WeekDay_Definitions */
  uint8_t hours;    /*!< Specifies the RTC Time Hour.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 12 if the RTC_HourFormat_12 is selected.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 23 if the RTC_HourFormat_24 is selected */
  uint8_t minutes;  /*!< Specifies the RTC Time Minutes.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 59 */
  uint8_t seconds;  /*!< Specifies the RTC Time Seconds.
                         This parameter must be a number between Min_Data = 0 and Max_Data = 59 */
}localRTC_t;
#pragma pack()

#define checkTimestamp(x) (((x)>=MIN_TIMESTAMP) ? 1 : 0)

uint32_t getTimeStamp(void);
void     setTimeStamp(uint32_t sec);
bool     initRTC(void);
void     setRTC(localRTC_t* rtc);
bool     getRTC(localRTC_t* rtc);
void     printRTC(localRTC_t* rtc);
uint32_t backsteppingTimestamp(uint32_t ms);
uint32_t getKernelTime_32(void);
s64_t    getKernelTime_64(void);
bool     timeIsAlreadySet(void);
uint32_t thrustTimestamp(uint32_t ms);
#endif