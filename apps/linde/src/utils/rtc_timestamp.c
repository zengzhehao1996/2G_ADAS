#include "rtc_timestamp.h"
#include <stdio.h>
#include <stdbool.h>

const unsigned char g_day_per_mon[MONTH_PER_YEAR] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


uint8_t isLeapYear(uint16_t year)
{
    if ((year % 400) == 0) 
    {
        return 1;
    } 
    else if ((year % 100) == 0) 
    {
        return 0;
    } 
    else if ((year % 4) == 0) 
    {
        return 1;
    } 
    else 
    {
        return 0;
    }
}

uint8_t dayOfMonth(uint8_t month, uint16_t year)
{
    if ((month == 0) || (month > 12)) 
    {
        return g_day_per_mon[1] + isLeapYear(year);
    }

    if (month != 2) 
    {
        return g_day_per_mon[month - 1];
    } 
    else 
    {
        return g_day_per_mon[1] + isLeapYear(year);
    }
}



uint32_t RTC2TimeStamp(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    uint16_t i;
    uint32_t no_of_days = 0;
    uint32_t utc_time;

    if (year < UTC_BASE_YEAR) 
    {
        return 0;
    }

    /* year */
    for (i = UTC_BASE_YEAR; i < year; i++) 
    {
        no_of_days += (DAY_PER_YEAR + isLeapYear(i));
    }

    /* month */
    for (i = 1; i < month; i++) 
    {
        no_of_days += dayOfMonth((uint8_t)i, year);
    }

    /* day */
    no_of_days += (day - 1);

    /* sec */
    utc_time = no_of_days * SEC_PER_DAY + (uint32_t) (hour*SEC_PER_HOUR+minute*SEC_PER_MIN+second);

    utc_time = utc_time - 3600*8;// BeiJing TimeZone repair

    return utc_time;
}

localRTC_t * timeStamp2RTC(const uint32_t srctime, localRTC_t *rtc)
{
    int n32_Pass4year,n32_hpery;

    if(!rtc){
        err_log("RTC Parama ERROR.");
        return NULL;
    }

    // 每个月的天数  非闰年
    const static char Days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // 一年的小时数
    const static int ONE_YEAR_HOURS = 8760; // 365 * 24 (非闰年)

    //计算时差8*60*60 固定北京时间
    uint32_t time = srctime;
    time=time+28800;
    if(time < 0)
    {
        time = 0;
    }

    //取秒时间
    rtc->seconds=(uint8_t)(time % 60);
    time /= 60;

    //取分钟时间
    rtc->minutes=(uint8_t)(time % 60);
    time /= 60;

    //计算星期
    rtc->weekDay=(uint8_t)((time/24+4)%7);
    if(0==rtc->weekDay)
    {
        rtc->weekDay = 7;
    }

    //取过去多少个四年，每四年有 1461*24 小时
    n32_Pass4year=(time / (1461L * 24L));

    //计算年份
    rtc->year=(uint8_t)((n32_Pass4year << 2)+70);

    //四年中剩下的小时数
    time %= 1461L * 24L;

    //计算在这一年的天数
    rtc->date=(uint8_t)((time/24)%365);

    //校正闰年影响的年份，计算一年中剩下的小时数
    for (;;)
    {
        //一年的小时数
        n32_hpery = ONE_YEAR_HOURS;

        //判断闰年
        if ((rtc->year & 3) == 0)
        {
            //是闰年，一年则多24小时，即一天
            n32_hpery += 24;
        }

        if (time < n32_hpery)
        {
            break;
        }

        rtc->year++;
        time -= n32_hpery;
    }

    if(rtc->year >= 100){
        rtc->year -= 100;
    }

    //小时数
    rtc->hours=(uint8_t)(time % 24);

    //一年中剩下的天数
    time /= 24;

    //假定为闰年
    time++;

    //校正润年的误差，计算月份，日期
    if ((rtc->year & 3) == 0)
    {
        if (time > 60)
        {
            time--;
        }
        else
        {
            if (time == 60)
            {
                rtc->month = 1;
                rtc->date = 29;

                rtc->month++;
                return rtc;
            }
        }
    }

    //计算月日
    for (rtc->month = 0;Days[rtc->month] < time;rtc->month++)
    {
        time -= Days[rtc->month];
    }

    rtc->date = (uint8_t)(time);

    rtc->month++;
    return rtc;
}



