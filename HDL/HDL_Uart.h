/**
 * @file HDL_Uart.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief change from HDL_USR_Uart_L476.h
 * @version 0.1
 * @date 2022-10-04
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
/*
 * HDL_USR_Uart_L476.h
 *
 *  Created on: 2019年3月12日
 *      Author: WangJianping
 *      抽象所有串口读写操作
 *      写操作-直接写串口
 *      读操作-从队列中直接读，队列大小1K
 *      初始化操作--在使能相关串口初始化参数,队列初始化
 *
 */

#ifndef HDL_USR_UART_H_
#define HDL_USR_UART_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"

// 串口号定义
typedef enum {
    COM1    = 1,
    COM2    = 2,
    COM3    = 3,
    COM4    = 4,
    COM5    = 5,
    COM6    = 6,
    COM_NUM = 6,
} COMID_t;

typedef void (*UartWriteOverCallback_t)(void *args);
typedef void (*UartReceiveCharCallback_t)(uint8_t ch);

void Uart_Init(COMID_t comId, uint32_t baud, uint32_t wordLen, uint32_t stopBit, uint32_t parity);
uint32_t Uart_Write(COMID_t comId, const uint8_t *writeBuf, uint32_t uLen);
uint32_t Uart_Read(COMID_t comId, uint8_t *pBuf, uint32_t uiLen);
uint32_t Uart_AvailableBytes(COMID_t comId);
uint32_t Uart_EmptyReadBuffer(COMID_t comId);
uint8_t Uart_SetWriteOverCallback(COMID_t comId, UartWriteOverCallback_t callback, void *args);
uint8_t Uart_RegisterReceiveCharCallback(COMID_t comId, UartReceiveCharCallback_t callback);
uint8_t Uart_UnregisterReceiveCharCallback(COMID_t comId);

#ifdef __cplusplus
}
#endif
#endif /* HDL_USR_UART_H_ */
