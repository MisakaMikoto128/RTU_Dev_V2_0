/**
 * @file HDL_G4_RTC_test.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-07
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#include "HDL_G4_RTC_test.h"
#include "log.h"
#include "mytime.h"
void HDL_G4_RTC_test()
{
	ulog_init_user();

	HDL_G4_RTC_Init();
	mytime_struct setTime;
	mytime_struct datetime;
	mytime_struct utc_datetime;
	uint16_t subsec = 0;
	uint32_t timestamp = 0;
	uint8_t week = 0;

	setTime.nYear = 2022;
	setTime.nMonth = 11;
	setTime.nDay = 03;
	setTime.nHour = 18;
	setTime.nMin = 9;
	setTime.nSec = 0;
	setTime.wSub = 0;
	//HDL_G4_RTC_SetStructTime(&setTime);
	while (1)
	{
		HAL_Delay(1000 - 1);

		//测试RTC_GetStructTime
		HDL_G4_RTC_GetStructTime(&datetime);
		Debug_Printf("\r\n");
		Debug_Printf("The time get by HDL_G4_RTC_GetStructTime:\r\n");
		Debug_Printf("%04d-%02d-%02d %02d:%02d:%02d %02d\r\n", datetime.nYear, datetime.nMonth, datetime.nDay, datetime.nHour, datetime.nMin, datetime.nSec, datetime.wSub);
		week = applib_dt_dayindex(datetime.nYear, datetime.nMonth, datetime.nDay);
		Debug_Printf("week : %u\r\n", (uint32_t)week);

		//测试RTC_GetTimeTick
		timestamp = HDL_G4_RTC_GetTimeTick(&subsec);
		Debug_Printf("The timestamp get by HDL_G4_RTC_GetTimeTick:\r\n");
		Debug_Printf("timestamp = %u, subsec = %02d\r\n", timestamp, subsec);
		
		//时区调整测试：假设RTC时间为本地时间
		utc_datetime = datetime;
		mytime_sub_hours(&utc_datetime,8);
		Debug_Printf("\r\n");
		Debug_Printf("The time get by HDL_G4_RTC_GetStructTime:\r\n");
		Debug_Printf("%04d-%02d-%02d %02d:%02d:%02d %02d\r\n", utc_datetime.nYear, utc_datetime.nMonth,\
		utc_datetime.nDay, utc_datetime.nHour, utc_datetime.nMin, utc_datetime.nSec, utc_datetime.wSub);
		week = applib_dt_dayindex(utc_datetime.nYear, utc_datetime.nMonth, utc_datetime.nDay);
		Debug_Printf("week : %u\r\n", (uint32_t)week);

	}
}

void HDL_G4_RTC_leap_year_test()
{
	ulog_init_user();

	HDL_G4_RTC_Init();
	mytime_struct setTime;
	mytime_struct datetime;
	uint16_t subsec = 0;
	uint32_t timestamp = 0;
	uint8_t week = 0;

	// CASE 1:
	//  setTime.nYear = 2000;
	//  setTime.nMonth = 2;
	//  setTime.nDay = 29;
	//  setTime.nHour = 23;
	//  setTime.nMin = 59;
	//  setTime.nSec = 58;
	//  setTime.wSub = 0;

	setTime.nYear = 2044;
	setTime.nMonth = 2;
	setTime.nDay = 28;
	setTime.nHour = 23;
	setTime.nMin = 59;
	setTime.nSec = 57;
	setTime.wSub = 0;
	HDL_G4_RTC_SetStructTime(&setTime);

	int n = 4;
	while (n--)
	{
		HAL_Delay(1000 - 1);

		HDL_G4_RTC_GetStructTime(&datetime);
		Debug_Printf("\r\n");
		Debug_Printf("%04d-%02d-%02d %02d:%02d:%02d %02d\r\n", datetime.nYear, datetime.nMonth, datetime.nDay, datetime.nHour, datetime.nMin, datetime.nSec, datetime.wSub);
		week = applib_dt_dayindex(datetime.nYear, datetime.nMonth, datetime.nDay);
		Debug_Printf("week : %u\r\n", (uint32_t)week);
	}
}

/**
 * @brief RTC 亚秒测试
 *
 */
void HDL_G4_RTC_subsecond_test()
{
	ulog_init_user();

	HDL_G4_RTC_Init();
	mytime_struct setTime;
	mytime_struct datetime;
	uint16_t subsec = 0;
	uint32_t timestamp = 0;
	uint8_t week = 0;

	setTime.nYear = 2044;
	setTime.nMonth = 2;
	setTime.nDay = 28;
	setTime.nHour = 23;
	setTime.nMin = 59;
	setTime.nSec = 57;
	setTime.wSub = 0;
	HDL_G4_RTC_SetStructTime(&setTime);

	uint32_t subsec_bak = 17;
	uint32_t sec_bak = 17;
	int n = 4;
	int subsec_cnt = 0; //经历了多少亚秒

	while (1)
	{
		HDL_G4_RTC_GetStructTime(&datetime);

		if (sec_bak != datetime.nSec)
		{
			Debug_Printf("\r\n");
			sec_bak = datetime.nSec;
			n--;
			if (n == 0)
			{
				break; //防止输出多余的亚秒
			}
		}

		if (subsec_bak != datetime.wSub)
		{
			subsec_bak = datetime.wSub;
			Debug_Printf("sec %u sub %02d sys:%u\r\n", datetime.nSec,datetime.wSub, HAL_GetTick());
			subsec_cnt++;
		}
	}
	Debug_Printf("It took %d subseconds", subsec_cnt);
	while (1)
	{
	}
}