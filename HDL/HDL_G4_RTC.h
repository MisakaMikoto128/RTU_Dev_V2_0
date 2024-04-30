/**
 * @file HDL_G4_RTC.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-05
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#ifndef HDL_G4_RTC_H
#define HDL_G4_RTC_H

#include "main.h"
#include "mytime.h"

#define RTC_SUBSEC_MAX 1024U
void HDL_G4_RTC_Init();
uint32_t HDL_G4_RTC_GetTimeTick(uint16_t *pSub);
void HDL_G4_RTC_GetStructTime(mytime_struct *myTime);
void HDL_G4_RTC_SetTimeTick(uint32_t localTime);
void HDL_G4_RTC_SetStructTime(mytime_struct *myTime);
#define HDL_G4_RTC_Subsec2mSec(subsec) ((uint32_t)((subsec)*(1000.0f/RTC_SUBSEC_MAX) + 0.5f))
#define HDL_G4_RTC_mSec2Subsec(ms) ((uint32_t)((ms)*RTC_SUBSEC_MAX*0.001f + 0.5f))
#endif // HDL_G4_RTC_H
