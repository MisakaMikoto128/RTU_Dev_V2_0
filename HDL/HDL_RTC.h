/**
 * @file HDL_RTC.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-05
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#ifndef HDL_RTC_H
#define HDL_RTC_H
#ifdef __cplusplus
extern "C" {
#endif
#include "mtime.h"

void HDL_RTC_Init();
uint64_t HDL_RTC_GetTimeTick(uint16_t *pSub);
uint64_t HDL_RTC_GetMsTimestamp();
void HDL_RTC_GetStructTime(mtime_t *pmtime);
void HDL_RTC_SetTimeTick(uint64_t timestamp);
void HDL_RTC_SetStructTime(mtime_t *pmtime);
bool HDL_RTC_HasSynced();

// 亚秒寄存器，部分MCU具有该寄存器
#if HDL_RTC_CLOCK_SOURCE_LSI == 0
#define RTC_SUBSEC_MAX 1024U
#else
#define RTC_SUBSEC_MAX 1000U
#endif

#define HDL_RTC_Subsec2mSec(subsec) ((uint64_t)((subsec) * (1000.0f / RTC_SUBSEC_MAX) + 0.5f))
#define HDL_RTC_mSec2Subsec(ms)     ((uint64_t)((ms) * RTC_SUBSEC_MAX * 0.001f + 0.5f))
#ifdef __cplusplus
}
#endif
#endif // HDL_RTC_H
