/**
 * @file HDL_CPU.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-01-11
 * @last modified 2023-01-11
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef HDL_CPU_H
#define HDL_CPU_H

#ifdef FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#endif // FREE_RTOS

#ifdef FREE_RTOS
/* 开关全局中断的宏 */
#define ENABLE_INT()  taskEXIT_CRITICAL()  /* 使能全局中断 */
#define DISABLE_INT() taskENTER_CRITICAL() /* 禁止全局中断 */
#else
/* 开关全局中断的宏 */
#define ENABLE_INT()  __set_PRIMASK(0) /* 使能全局中断 */
#define DISABLE_INT() __set_PRIMASK(1) /* 禁止全局中断 */
#endif                                 // FREE_RTOS

#endif //! HDL_CPU_H