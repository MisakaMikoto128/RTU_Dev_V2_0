/**
 * @file CHIP_EC800M.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-09-16
 * @last modified 2023-09-16
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef CHIP_EC800M_H
#define CHIP_EC800M_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "HDL_G4_Uart.h"

#define RST_4G_Pin LL_GPIO_PIN_3
#define RST_4G_GPIO_Port GPIOE
#define PEN_4G_Pin LL_GPIO_PIN_2
#define PEN_4G_GPIO_Port GPIOE
#define DTR_4G_Pin LL_GPIO_PIN_4
#define DTR_4G_GPIO_Port GPIOA
#define RI_4G_Pin LL_GPIO_PIN_5
#define RI_4G_GPIO_Port GPIOA

#define CHIP_EC800M_COM COM2

    /**
     * @brief 用于初始化与EC800M模块通信的接口，即控制IO以及串口初始化
     *
     */
    void CHIP_EC800M_Init();
    uint32_t CHIP_EC800M_Write(const uint8_t *writeBuf, uint32_t uLen);
    uint32_t CHIP_EC800M_Read(uint8_t *pBuf, uint32_t uiLen);
    uint32_t CHIP_EC800M_AvailableBytes();
    uint32_t CHIP_EC800M_EmptyReadBuffer();

    void CHIP_EC800M_PowerOn();
    void CHIP_EC800M_PowerOff();
    void CHIP_EC800M_WakeupOff();
    void CHIP_EC800M_WakeupOn();
    void CHIP_EC800M_ResetOn();
    void CHIP_EC800M_ResetOff();
#ifdef __cplusplus
}
#endif
#endif //! CHIP_EC800M_H
