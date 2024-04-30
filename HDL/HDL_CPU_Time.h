/**
 * @file HDL_CPU_Time.h
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

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

typedef void (*CPU_Time_Callback_t)(void);

#define US_TIMER_BITWIDE 32 // 32或者16bit
#if US_TIMER_BITWIDE == 16
#define CPU_MAX_US_DELAY 0xFFFFU
typedef uint16_t UsTimer_t;
#elif US_TIMER_BITWIDE == 32
#define CPU_MAX_US_DELAY 0xFFFFFFFFU
typedef uint32_t UsTimer_t;
#endif

void HDL_CPU_Time_Init();
uint32_t HDL_CPU_Time_GetTick();
void HDL_CPU_Time_DelayMs(uint32_t DelayMs);
void HDL_CPU_Time_ResetTick();
uint32_t HDL_CPU_Time_GetUsTick();
void HDL_CPU_Time_ResetUsTick();

void HDL_CPU_Time_DelayUs(uint32_t DelayUs);
void HDL_CPU_Time_StartHardTimer(uint8_t _CC, UsTimer_t _uiTimeOut, void *_pCallBack);
void HDL_CPU_Time_StopHardTimer(uint8_t _CC);
void HDL_CPU_Time_SetCPUTickCallback(CPU_Time_Callback_t _pCallBack);

#define HDL_CPU_TIME_OEN_TICK_TIME 1000ULL // 1000 us
/**
 * @brief 将时间转换为tick,time单位为ms,
 *
 */
#define HDL_TIME_TO_TICK(_time) ((((_time) * 1000ULL) / HDL_CPU_TIME_OEN_TICK_TIME))

/**
 * @brief 将tick转换为时间,返回值单位为ms
 *
 */
#define HDL_TICK_TO_TIME(_tick) ((((_time) * HDL_CPU_TIME_OEN_TICK_TIME) / 1000ULL))

#ifdef __cplusplus
}
#endif

#endif // !CPU_TIME_H
