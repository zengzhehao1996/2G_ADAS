#include "rtc.h"
#include "rtc_timestamp.h"
#include <stm32f4xx.h>
#include <stm32f4xx_hal_rtc.h>
#include <stm32f4xx_hal_rtc_ex.h>

static struct 
{
    uint32_t serverTs;
    uint32_t localOffset;
}sysTs;

static bool g_time_already = false;

RTC_HandleTypeDef RTC_Handler;

HAL_StatusTypeDef RTC_Set_Time(uint8_t hour,uint8_t min,uint8_t sec,uint8_t ampm);
HAL_StatusTypeDef RTC_Set_Date(uint8_t year,uint8_t month,uint8_t date,uint8_t week);

void     restartSetTimeStamp(uint32_t sec);

uint32_t getTimeStamp(void)
{
    if(!checkTimestamp(sysTs.serverTs))
    {
        return 0;
    }
    return (sysTs.serverTs+k_uptime_get_32()/1000-sysTs.localOffset);
}

void     restartSetTimeStamp(uint32_t sec)
{
    if(0==checkTimestamp(sec))
    {
        err_log("Timestamp less than compile time! return. ^^^^ \n");
        return;
    }
    sysTs.serverTs = sec;
    sysTs.localOffset = k_uptime_get_32()/1000;
}

void     setTimeStamp(uint32_t sec)
{
    if(0==checkTimestamp(sec))
    {
        err_log("Timestamp less than compile time! return. ^^^^ \n");
        return;
    }
    sysTs.serverTs = sec;
    sysTs.localOffset = k_uptime_get_32()/1000;
    print_log("SET timestamp :[%u] ==================================\n",sec);
    g_time_already = true;
}

bool timeIsAlreadySet(void)
{
    return g_time_already;
}

bool     initRTC(void)
{
    /* init time stamp */

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    
#ifdef RTC_USE_LSI
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
#endif    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        printk("HAL_RCC_OscConfig error\n");
    }
    
    __HAL_RCC_PWR_CLK_ENABLE();

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
#ifdef RTC_USE_LSI
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
#else
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
#endif    
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        printk("RTC Clock config error!\n");
    }
    
    RTC_Handler.Instance = RTC;
    RTC_Handler.Init.HourFormat = RTC_HOURFORMAT_24;
    RTC_Handler.Init.AsynchPrediv = 127;
    RTC_Handler.Init.SynchPrediv = 255;
    RTC_Handler.Init.OutPut = RTC_OUTPUT_DISABLE;
    RTC_Handler.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    RTC_Handler.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&RTC_Handler) != HAL_OK)
    {
        printk("[%s,%d]fail to init rtc:0x%x\n",__FILE__,__LINE__,RTC_Handler.State);
        return false;
    }
    //printk("[%s,%d]initialzation rtc done\n",__FILE__,__LINE__);

    localRTC_t rtc;
    if(getRTC(&rtc))
    {
        uint16_t year = 2000 + rtc.year;
        uint32_t timestamp = RTC2TimeStamp(year,rtc.month,rtc.date,rtc.hours,rtc.minutes,rtc.seconds);
        restartSetTimeStamp(timestamp);
        print_log("\n\tRTC To timestamp:[%u].\n", timestamp);
    }
    else
    {
        warning_log("Don't get RTC.\n");
    }

    return true;;
};



void     setRTC(localRTC_t* rtc)
{
    if(rtc==NULL)return;

    RTC_Set_Time(rtc->hours, rtc->minutes, rtc->seconds, RTC_HOURFORMAT_24);
    RTC_Set_Date(rtc->year, rtc->month, rtc->date, rtc->weekDay);
    print_log("======== Write RTC OK. ========\n");
}

bool     getRTC(localRTC_t* rtc)
{
    if(rtc==NULL)return false;
    RTC_TimeTypeDef s_time;
    RTC_DateTypeDef s_date;

    //step1. get time
    HAL_RTC_GetTime(&RTC_Handler, &s_time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&RTC_Handler, &s_date, RTC_FORMAT_BIN);
    //printk("[%s,%d]hrs:%d,mins:%d, sec:%d\n",__FILE__,__LINE__,s_time.Hours,s_time.Minutes,s_time.Seconds);

    //step2. fill the out struct
    rtc->year = s_date.Year;
    rtc->month = s_date.Month;
    rtc->date = s_date.Date;
    rtc->weekDay = s_date.WeekDay;
    rtc->hours = s_time.Hours;
    rtc->minutes = s_time.Minutes;
    rtc->seconds = s_time.Seconds;

    return true;
}

void     printRTC(localRTC_t* rtc)
{
    if(!rtc)
    {
        return ;
    }
    print_log("\n\tRTC:\t20%02d-%02d-%02d %02d:%02d:%02d [ %d week ]\n"
                ,rtc->year, rtc->month, rtc->date
                ,rtc->hours, rtc->minutes, rtc->seconds, rtc->weekDay);
}

uint32_t getKernelTime_32(void)
{
    return k_uptime_get_32();
}

s64_t    getKernelTime_64(void)
{
    return k_uptime_get_32();
}

HAL_StatusTypeDef RTC_Set_Time(uint8_t hour,uint8_t min,uint8_t sec,uint8_t ampm)
{
    RTC_TimeTypeDef RTC_TimeStructure;

    RTC_TimeStructure.Hours=hour;
    RTC_TimeStructure.Minutes=min;
    RTC_TimeStructure.Seconds=sec;
    RTC_TimeStructure.TimeFormat=ampm;
    RTC_TimeStructure.DayLightSaving=RTC_DAYLIGHTSAVING_NONE;
    RTC_TimeStructure.StoreOperation=RTC_STOREOPERATION_RESET;
    return HAL_RTC_SetTime(&RTC_Handler,&RTC_TimeStructure,RTC_FORMAT_BIN);
}

HAL_StatusTypeDef RTC_Set_Date(uint8_t year,uint8_t month,uint8_t date,uint8_t week)
{
    RTC_DateTypeDef RTC_DateStructure;
        
    RTC_DateStructure.Date=date;
    RTC_DateStructure.Month=month;
    RTC_DateStructure.WeekDay=week;
    RTC_DateStructure.Year=year;
    return HAL_RTC_SetDate(&RTC_Handler,&RTC_DateStructure,RTC_FORMAT_BIN);
}

void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    RCC_OscInitStruct.OscillatorType=RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.PLL.PLLState=RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState=RCC_LSE_ON;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    PeriphClkInitStruct.PeriphClockSelection=RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection=RCC_RTCCLKSOURCE_LSE;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
        
    __HAL_RCC_RTC_ENABLE();
}

uint32_t thrustTimestamp(uint32_t ms)
{
    uint32_t thruTs = 0;
    uint32_t curMs = k_uptime_get_32();
    uint32_t curTs = getTimeStamp();
    
    thruTs = curTs - (curMs-ms)/1000;
    print_log("curTs:[%d],thruTs:[%d],minTs:[%d],curms:[%d],ms:[%d]\n",curTs,thruTs,MIN_TIMESTAMP,curMs,ms);
    if((checkTimestamp(thruTs)))
    {
        print_log("thruTs:[%d]\n",thruTs);
        return thruTs;
    }
    else
    {
        print_log("thruTs:[%d]\n",thruTs);
        return 0;
    }
    
}