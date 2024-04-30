/**
 * @file HDL_G4_RTC.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-07
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

/**
 * @brief 没有时区概念在这个RTC库中，本地时间当作UTC时间。
 *
 */
//存在一定的问题，ST的这个RTC没有一个单独的秒寄存器，就是一个DateTime日历，
//对于闰年的处理取决于RTC_BASE_YEAR,但是RTC_BASE_YEAR手册中却从来没有提及。

#include "HDL_G4_RTC.h"
#include "mytime.h"

#define RTC_TR_RESERVED_MASK (RTC_TR_PM | RTC_TR_HT | RTC_TR_HU |   \
                              RTC_TR_MNT | RTC_TR_MNU | RTC_TR_ST | \
                              RTC_TR_SU)
#define RTC_DR_RESERVED_MASK (RTC_DR_YT | RTC_DR_YU | RTC_DR_WDU | \
                              RTC_DR_MT | RTC_DR_MU | RTC_DR_DT |  \
                              RTC_DR_DU)

/* rtc base year , it is crucial to judge leap years */
#define RTC_BASE_YEAR 2000U
#define RTC_BASE_MONTH 1U
#define RTC_BASE_DAY 1U
#define RTC_BASE_HOUR 0U
#define RTC_BASE_MINUTE 0U
#define RTC_BASE_SECOND 0U

/**
 * @brief RTC外设初始化。启动RTC，但是不会设置RTC时间。默认LSE时钟源。
 * 设置/获取时间默认使用BIN格式的时间。
 *
 */
void HDL_G4_RTC_Init()
{
    LL_RTC_InitTypeDef RTC_InitStruct = {0};
    mytime_struct setTime;

    //设置RTC时钟源为LSE
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE)
    {
        //使能后备寄存器访问
        /* Update LSE configuration in Backup Domain control register */
        /* Requires to enable write access to Backup Domain if necessary */
        if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_PWR) != 1U)
        {
            /* Enables the PWR Clock and Enables access to the backup domain */
            //打开RTC后备寄存器电源时钟
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
        }
        if (LL_PWR_IsEnabledBkUpAccess() != 1U)
        {
            /* Enable write access to Backup domain */
            //打开后备寄存器访问
            LL_PWR_EnableBkUpAccess();
            while (LL_PWR_IsEnabledBkUpAccess() == 0U)
            {
            }
        }

        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();

        //使能LSE
        LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
        LL_RCC_LSE_Enable();
        /* Wait till LSE is ready */
        while (LL_RCC_LSE_IsReady() != 1)
        {
        }

        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

        /* Restore clock configuration if changed */

        LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
    }

    //清除RSF位，防止例如从低功耗模式（停机模式或待机模式）唤醒之后带来的不同步。当然是对于等待RSF位的程序才会。
    LL_RTC_ClearFlag_RS(RTC);

    // BYPSHAD = 1可以直接读取，但是如果在对寄存器的两次读访问之间出现 RTCCLK 沿，则不同寄存器的结果彼此可能不一致
    //		LL_RTC_DisableWriteProtection(RTC);
    //		LL_RTC_EnableShadowRegBypass(RTC);
    //		LL_RTC_EnableWriteProtection(RTC);

    //使能RTC时钟
    LL_RCC_EnableRTC();
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTCAPB);

    //初始化RTC
    RTC_InitStruct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
    // RTC_InitStruct.AsynchPrescaler = 128 - 1;
    // RTC_InitStruct.SynchPrescaler = 256 - 1; // sub-second = 1 sec / 256
    RTC_InitStruct.AsynchPrescaler = 32 - 1;
    RTC_InitStruct.SynchPrescaler = RTC_SUBSEC_MAX - 1; // sub-second = 1 sec / RTC_SUBSEC_MAX
    LL_RTC_Init(RTC, &RTC_InitStruct);
#define BAK_VALUE 0x32F2
    if (LL_RTC_BKP_GetRegister(RTC, LL_RTC_BKP_DR0) != BAK_VALUE)
    {
        setTime.nYear = 2022;
        setTime.nMonth = 12;
        setTime.nDay = 20;
        setTime.nMin = 25;
        setTime.nHour = 14;
        setTime.nSec = 0;
        setTime.wSub = 0;
        HDL_G4_RTC_SetStructTime(&setTime);
        LL_RTC_BKP_SetRegister(RTC, LL_RTC_BKP_DR0, BAK_VALUE);
    }
}

/**
 * @brief 获取本地1970-1-1以来总秒数
 *
 * @param pSub 用于获取亚秒数。
 * @return uint32_t
 */
uint32_t HDL_G4_RTC_GetTimeTick(uint16_t *pSub)
{
    uint32_t timestamp = 0;
    mytime_struct datetime;
    HDL_G4_RTC_GetStructTime(&datetime);
    timestamp = mytime_2_utc_sec(&datetime);
    *pSub = datetime.wSub;
    return timestamp;
}

/**
 * @brief 获取mytime_struct类型的时间。
 *
 * @param myTime
 */
void HDL_G4_RTC_GetStructTime(mytime_struct *myTime)
{
    // using volatile to avoid optimizing local variables
    __IO uint32_t TR; /*!< RTC time register,                                         Address offset: 0x00 */
    __IO uint32_t DR; /*!< RTC date register,                                         Address offset: 0x04 */
    __IO uint32_t SSR;

    SSR = (uint32_t)(READ_BIT(RTC->SSR, RTC_SSR_SS)); //会锁住DR，能够保证时间日期一定是一致的，亚秒不能保证，这个要确保
    // SSR和TR读取时间间隔足够小，不过时间中精度只有秒的场合不影响。
    //先读取一下TR和DR让时间和日期更新到影子寄存器
    TR = (uint32_t)(READ_REG(RTC->TR) & RTC_TR_RESERVED_MASK);
    DR = (uint32_t)(READ_REG(RTC->DR) & RTC_DR_RESERVED_MASK);

    myTime->nHour = __LL_RTC_CONVERT_BCD2BIN((uint8_t)((TR & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos));
    myTime->nMin = __LL_RTC_CONVERT_BCD2BIN((uint8_t)((TR & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos));
    myTime->nSec = __LL_RTC_CONVERT_BCD2BIN((uint8_t)((TR & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_SU_Pos));

    myTime->wSub = RTC_SUBSEC_MAX - SSR;

    myTime->nYear = __LL_RTC_CONVERT_BCD2BIN((uint8_t)((DR & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos)) + RTC_BASE_YEAR;
    myTime->nMonth = __LL_RTC_CONVERT_BCD2BIN((uint8_t)((DR & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos));
    myTime->nDay = __LL_RTC_CONVERT_BCD2BIN((uint8_t)((DR & (RTC_DR_DT | RTC_DR_DU)) >> RTC_DR_DU_Pos));
}

/**
 * @brief 使用时间戳设置日历时间。
 *
 * @param localTime 1970-1-1以来总秒数。
 */
void HDL_G4_RTC_SetTimeTick(uint32_t localTime)
{
    mytime_struct datetime;
    utc_sec_2_mytime(localTime, &datetime);
    HDL_G4_RTC_SetStructTime(&datetime);
}

/**
 * @brief 使用mytime_struct对象来设置时间。被设置的时间必须大于等于RTC_BASE_YEAR
 * 否则没有效果。设置时间的时间精度为秒。
 *
 * @param myTime
 */
void HDL_G4_RTC_SetStructTime(mytime_struct *myTime)
{
    LL_RTC_TimeTypeDef RTC_TimeStruct = {0};
    LL_RTC_DateTypeDef RTC_DateStruct = {0};

    RTC_TimeStruct.Hours = myTime->nHour;
    RTC_TimeStruct.Minutes = myTime->nMin;
    RTC_TimeStruct.Seconds = myTime->nSec;

    LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_TimeStruct);
    RTC_DateStruct.Month = myTime->nMonth;
    RTC_DateStruct.Day = myTime->nDay;
    RTC_DateStruct.Year = myTime->nYear - RTC_BASE_YEAR;
    RTC_DateStruct.WeekDay = applib_dt_dayindex(myTime->nYear, myTime->nMonth, myTime->nDay);

    LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_DateStruct);
}