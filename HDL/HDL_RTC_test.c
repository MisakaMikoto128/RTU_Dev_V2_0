/**
 * @file HDL_RTC_test.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-07
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#include "HDL_CPU_Time.h"
#include "HDL_RTC_test.h"
#include "log.h"
#include "mtime.h"
#include "scheduler.h"
void HDL_RTC_test()
{
    ulog_init_user();

    HDL_RTC_Init();
    mtime_t setTime;
    mtime_t datetime;
    mtime_t utc_datetime;
    uint16_t subsec    = 0;
    uint32_t timestamp = 0;
    uint8_t week       = 0;

    setTime.nYear  = 2022;
    setTime.nMonth = 11;
    setTime.nDay   = 03;
    setTime.nHour  = 18;
    setTime.nMin   = 9;
    setTime.nSec   = 0;
    setTime.wSub   = 0;
    // HDL_RTC_SetStructTime(&setTime);
    while (1) {
        HDL_CPU_Time_DelayMs(1000 - 1);

        // 测试RTC_GetStructTime
        HDL_RTC_GetStructTime(&datetime);
        Debug_Printf("\r\n");
        Debug_Printf("The time get by HDL_RTC_GetStructTime:\r\n");
        Debug_Printf("%04d-%02d-%02d %02d:%02d:%02d %02d\r\n", datetime.nYear, datetime.nMonth, datetime.nDay, datetime.nHour, datetime.nMin, datetime.nSec, datetime.wSub);
        week = mtime_get_week(datetime.nYear, datetime.nMonth, datetime.nDay);
        Debug_Printf("week : %u\r\n", (uint32_t)week);

        // 测试RTC_GetTimeTick
        timestamp = HDL_RTC_GetTimeTick(&subsec);
        Debug_Printf("The timestamp get by HDL_RTC_GetTimeTick:\r\n");
        Debug_Printf("timestamp = %u, subsec = %02d\r\n", timestamp, subsec);

        // 时区调整测试：假设RTC时间为本地时间
        utc_datetime = datetime;
        mtime_sub_hours(&utc_datetime, 8);
        Debug_Printf("\r\n");
        Debug_Printf("The time get by HDL_RTC_GetStructTime:\r\n");
        Debug_Printf("%04d-%02d-%02d %02d:%02d:%02d %02d\r\n", utc_datetime.nYear, utc_datetime.nMonth,
                     utc_datetime.nDay, utc_datetime.nHour, utc_datetime.nMin, utc_datetime.nSec, utc_datetime.wSub);
        week = mtime_get_week(utc_datetime.nYear, utc_datetime.nMonth, utc_datetime.nDay);
        Debug_Printf("week : %u\r\n", (uint32_t)week);
    }
}

void HDL_RTC_leap_year_test()
{
    ulog_init_user();

    HDL_RTC_Init();
    mtime_t setTime;
    mtime_t datetime;
    uint16_t subsec    = 0;
    uint32_t timestamp = 0;
    uint8_t week       = 0;

    // CASE 1:
    //  setTime.nYear = 2000;
    //  setTime.nMonth = 2;
    //  setTime.nDay = 29;
    //  setTime.nHour = 23;
    //  setTime.nMin = 59;
    //  setTime.nSec = 58;
    //  setTime.wSub = 0;

    setTime.nYear  = 2044;
    setTime.nMonth = 2;
    setTime.nDay   = 28;
    setTime.nHour  = 23;
    setTime.nMin   = 59;
    setTime.nSec   = 57;
    setTime.wSub   = 0;
    HDL_RTC_SetStructTime(&setTime);

    int n = 4;
    while (n--) {
        HDL_CPU_Time_DelayMs(1000 - 1);

        HDL_RTC_GetStructTime(&datetime);
        Debug_Printf("\r\n");
        Debug_Printf("%04d-%02d-%02d %02d:%02d:%02d %02d\r\n", datetime.nYear, datetime.nMonth, datetime.nDay, datetime.nHour, datetime.nMin, datetime.nSec, datetime.wSub);
        week = mtime_get_week(datetime.nYear, datetime.nMonth, datetime.nDay);
        Debug_Printf("week : %u\r\n", (uint32_t)week);
    }
}

/**
 * @brief RTC 亚秒测试
 *
 */
void HDL_RTC_subsecond_test()
{

    // 基础外设初始化
    HDL_CPU_Time_Init();
    HDL_RTC_Init();

    // 基本功能初始化
    ulog_init_user();

    mtime_t setTime;
    mtime_t datetime;
    uint16_t subsec    = 0;
    uint32_t timestamp = 0;
    uint8_t week       = 0;

    setTime.nYear  = 2044;
    setTime.nMonth = 2;
    setTime.nDay   = 28;
    setTime.nHour  = 23;
    setTime.nMin   = 59;
    setTime.nSec   = 57;
    setTime.wSub   = 0;
    HDL_RTC_SetStructTime(&setTime);

    uint32_t subsec_bak = 17;
    uint32_t sec_bak    = 17;
    int n               = 4;
    int subsec_cnt      = 0; // 经历了多少亚秒

    // while (1)
    // {
    // 	HDL_RTC_GetStructTime(&datetime);

    // 	if (sec_bak != datetime.nSec)
    // 	{
    // 		Debug_Printf("\r\n");
    // 		sec_bak = datetime.nSec;
    // 		n--;
    // 		if (n == 0)
    // 		{
    // 			break; // 防止输出多余的亚秒
    // 		}
    // 	}

    // 	if (subsec_bak != datetime.wSub)
    // 	{
    // 		subsec_bak = datetime.wSub;
    // 		Debug_Printf("sec %u sub %02d ms %02d sys_ms:%u diff:%d\r\n", datetime.nSec, datetime.wSub, HDL_RTC_Subsec2mSec(datetime.wSub), HAL_GetTick(), HAL_GetTick() - HDL_RTC_Subsec2mSec(datetime.wSub));
    // 		subsec_cnt++;
    // 	}
    // }
    // Debug_Printf("It took %d subseconds", subsec_cnt);

    uint64_t rtc_ms     = 0;
    uint64_t rtc_ms_bak = 0;
    while (1) {
        // if (period_query(1, 50))
        {
            uint32_t datetime; // 日期时间UTC时间戳：精度秒，24小时制。
            uint16_t ms;       // 毫秒
            datetime        = HDL_RTC_GetTimeTick(&ms);
            rtc_ms          = datetime * 1000 + HDL_RTC_Subsec2mSec(ms);
            uint32_t cpu_ms = HDL_CPU_Time_GetTick();
            if (rtc_ms_bak != rtc_ms) {
                rtc_ms_bak = rtc_ms;
                ULOG_DEBUG("rtc_ms %llu ms sys_ms:%u diff:%llu", rtc_ms, cpu_ms, rtc_ms - cpu_ms);
            }

            // {
            // 	uint32_t datetime; // 日期时间UTC时间戳：精度秒，24小时制。
            // 	uint16_t ms;	   // 毫秒
            // 	datetime = HDL_RTC_GetTimeTick(&ms);
            // 	uint32_t rtc_ms = datetime * 1000 + ms;

            // 	if (rtc_ms_bak != rtc_ms)
            // 	{
            // 		rtc_ms_bak = rtc_ms;
            // 		uint32_t current_tick = HDL_CPU_Time_GetTick();
            // 		Debug_Printf("[RTU5]->%u %u %u\r\n", rtc_ms, current_tick, rtc_ms - current_tick);
            // 	}
            // }
        }
    }
}