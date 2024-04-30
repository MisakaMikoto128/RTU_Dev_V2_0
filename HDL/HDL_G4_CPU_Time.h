/**
 * @file HDL_G4_CPU_Time.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2022-10-08
 * 
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 * 
 */

#ifndef CPU_TIME_H
#define CPU_TIME_H
#include "main.h"

typedef void(*CPU_Time_Callback_t)(void);

void HDL_G4_CPU_Time_Init();
uint32_t HDL_G4_CPU_Time_GetTick();
void HDL_G4_CPU_Time_ResetTick();
uint32_t HDL_G4_CPU_Time_GetUsTick();
void HDL_G4_CPU_Time_ResetUsTick();

void HDL_CPU_Time_DelayMs(uint32_t DelayMs);
void HDL_G4_CPU_Time_DelayUs(uint32_t DelayUs);
void HDL_G4_CPU_Time_StartHardTimer(uint8_t _CC, uint32_t _uiTimeOut, void *_pCallBack);
void HDL_G4_CPU_Time_StopHardTimer(uint8_t _CC);
void HDL_G4_CPU_Time_RegisterTickCallback(CPU_Time_Callback_t callback);
#endif // !CPU_TIME_H

