/**
 * @file HDL_GPIO.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-01-16
 * @last modified 2023-01-16
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef HDL_GPIO_H
#define HDL_GPIO_H

#include "main.h"
void HDL_GPIO_EnablePeripheralCLK(GPIO_TypeDef *GPIOx);

// 定义GPIO高低电平
typedef enum {
    HDL_GPIO_LOW  = 0,
    HDL_GPIO_HIGH = 1
} HDL_GPIO_PinState_t;

#endif //! HDL_GPIO_H