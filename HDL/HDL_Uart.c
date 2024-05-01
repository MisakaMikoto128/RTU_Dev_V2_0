/**
 * @file HDL_Uart.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-04
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#include "HDL_Uart.h"
#include "HDL_CPU.h"
#include "cqueue.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"

typedef struct tagCOM_Dev_t {
    COMID_t comId;
    // 主要是给Modbus使用，因为使用FIFO，发送完成时机不等于释放总线的时机。
    // 其时机是连续发送一串，然后直到硬件FIFO中的数据完全发送出去了产生回调，回调属于中断函数。
    // 这里实现依赖于串口的TC中断。
    // 这意味着如果两个Uart_Write对同一个串口写入的时机过于接近，那么就无法使得一次Uart_Write
    // 产生一个回调。
    UartWriteOverCallback_t write_over_callback;
    void *write_over_callback_args;
    UartReceiveCharCallback_t receive_char_callback;
    uint32_t baud;
    uint32_t wordLen;
    uint32_t stopBit;
    uint32_t parity;
    CQueue_t txQueue;
    CQueue_t rxQueue;
    uint8_t *txBuf;
    uint8_t *rxBuf;
    // 串口是否初始化
    bool inited;
} COM_Dev_t;

/**************************
 *
 *
优先级：
NVIC_PRIORITYGROUP_4 :
    4 bits for pre-emption priority,
    0 bit  for subpriority

COM map:
COM1 <---> USART1
COM2 <---> USART2
COM3 <---> USART3
COM4 <---> UART4
COM5 <---> UART5
COM6 <---> LPUART1

PIN Map:

USART1
    PA9   ------> USART1_TX
    PA10   ------> USART1_RX
USART2
    PA15   ------> USART2_RX
    PB3   ------> USART2_TX
USART3
    PB8-BOOT0   ------> USART3_RX
    PB9   ------> USART3_TX
UART4
    PC10   ------> UART4_TX
    PC11   ------> UART4_RX
UART5
    PC12   ------> UART5_TX
    PD2   ------> UART5_RX
LPUART1
    PB10   ------> LPUART1_RX
    PB11   ------> LPUART1_TX


G473:
USART1
    PA9   ------> USART1_TX
    PA10  ------> USART1_RX
USART2
    PD6   ------> USART2_RX
    PD5   ------> USART2_TX
USART3
    PD9   ------> USART3_RX
    PD8   ------> USART3_TX
UART4
    PC10  ------> UART4_TX
    PC11  ------> UART4_RX
UART5
    PD2  ------> UART5_TX
    PC12   ------> UART5_RX
LPUART1
    PC0   ------> LPUART1_RX
    PC1   ------> LPUART1_TX

    这个串口库的发送全部启用了FIFO，大小为8，并且启用了TX FIFO empty中断。
    如果对字符实时性要求高的场合FIFO可能会产生不好的影响，需要自己修改。
**************************/

// 串口设备抽象
COM_Dev_t _gCOMList[COM_NUM] = {0};

// 串口1相关变量
#define COM1_RX_BUF_SIZE 200
static uint8_t m_Com1RxBuf[COM1_RX_BUF_SIZE] = {0};
#define COM1_TX_BUF_SIZE (1024)
static uint8_t m_Com1TxBuf[COM1_TX_BUF_SIZE] = {0};

// 串口2相关变量
#define COM2_RX_BUF_SIZE 1024
static uint8_t m_Com2RxBuf[COM2_RX_BUF_SIZE] = {0};
#define COM2_TX_BUF_SIZE 1024
static uint8_t m_Com2TxBuf[COM2_TX_BUF_SIZE] = {0};

// 串口3相关变量
#define COM3_RX_BUF_SIZE 100
static uint8_t m_Com3RxBuf[COM3_RX_BUF_SIZE] = {0};
#define COM3_TX_BUF_SIZE 100
static uint8_t m_Com3TxBuf[COM3_TX_BUF_SIZE] = {0};

// 串口4相关变量
#define COM4_RX_BUF_SIZE 100
static uint8_t m_Com4RxBuf[COM4_RX_BUF_SIZE] = {0};
#define COM4_TX_BUF_SIZE 50
static uint8_t m_Com4TxBuf[COM4_TX_BUF_SIZE] = {0};

// 串口5相关变量
#define COM5_RX_BUF_SIZE 100
static uint8_t m_Com5RxBuf[COM5_RX_BUF_SIZE] = {0};
#define COM5_TX_BUF_SIZE 100
static uint8_t m_Com5TxBuf[COM5_TX_BUF_SIZE] = {0};

// 串口6相关变量
#define COM6_RX_BUF_SIZE 100
static uint8_t m_Com6RxBuf[COM6_RX_BUF_SIZE] = {0};
#define COM6_TX_BUF_SIZE 100
static uint8_t m_Com6TxBuf[COM6_TX_BUF_SIZE] = {0};

/**
 * @brief 串口初始化
 *
 * @param comx 串口号
 * @param baud 波特率
 * @param wordLen 数据宽度 LL_USART_DATAWIDTH_7B,LL_USART_DATAWIDTH_8B,LL_USART_DATAWIDTH_9B
 * @param stopBit 停止位个数LL_USART_STOPBITS_1,LL_USART_STOPBITS_2
 * @param parity 奇偶校验位LL_USART_PARITY_NONE LL_USART_PARITY_EVEN LL_USART_PARITY_ODD
 * @note 如果是串口是LPUART，那么推荐使用的初始化参数名字如下：
 * wordLen-LL_LPUART_DATAWIDTH_7B,LL_LPUART_DATAWIDTH_8B,LL_LPUART_DATAWIDTH_9B
 * stopBit-LL_LPUART_STOPBITS_1,LL_LPUART_STOPBITS_2
 * parity-LL_LPUART_PARITY_NONE,LL_LPUART_PARITY_EVEN,LL_LPUART_PARITY_ODD
 */
void Uart_Init(COMID_t comId, uint32_t baud, uint32_t wordLen, uint32_t stopBit,
               uint32_t parity)
{
    if (comId >= COM_NUM) {
        return;
    }
    COM_Dev_t *pDev = &_gCOMList[comId];
    pDev->comId     = comId;
    pDev->baud      = baud;
    pDev->wordLen   = wordLen;
    pDev->stopBit   = stopBit;
    pDev->parity    = parity;

    LL_GPIO_InitTypeDef GPIO_InitStruct   = {0};
    LL_USART_InitTypeDef USART_InitStruct = {0};
    switch (comId) {
        case COM1: {
            if (pDev->inited == false) {
                pDev->txBuf = (uint8_t *)m_Com1TxBuf;
                pDev->rxBuf = (uint8_t *)m_Com1RxBuf;
                cqueue_create(&pDev->txQueue, pDev->txBuf, COM1_TX_BUF_SIZE, sizeof(uint8_t));
                cqueue_create(&pDev->rxQueue, pDev->rxBuf, COM1_RX_BUF_SIZE, sizeof(uint8_t));
            }

            LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);
            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
            /**USART1 GPIO Configuration
             PA9   ------> USART1_TX
             PA10   ------> USART1_RX
             */
            GPIO_InitStruct.Pin        = LL_GPIO_PIN_9 | LL_GPIO_PIN_10;
            GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate  = LL_GPIO_AF_7;
            LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
            NVIC_SetPriority(USART1_IRQn,
                             NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
            NVIC_EnableIRQ(USART1_IRQn);
            USART_InitStruct.BaudRate            = baud;
            USART_InitStruct.DataWidth           = wordLen;
            USART_InitStruct.StopBits            = stopBit;
            USART_InitStruct.Parity              = parity;
            USART_InitStruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
            USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
            USART_InitStruct.OverSampling        = LL_USART_OVERSAMPLING_16;
            LL_USART_Init(USART1, &USART_InitStruct);

            LL_USART_SetTXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_SetRXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_EnableFIFO(USART1);

            LL_USART_ConfigAsyncMode(USART1);
            LL_USART_Enable(USART1);
            LL_USART_EnableIT_RXNE(USART1); // 接收中断
            LL_USART_EnableIT_PE(USART1);   // 奇偶校验错误中断

            // LL_USART_EnableIT_TXFE(USART1); //启用TXFIFO Empty中断
            LL_USART_DisableIT_TC(USART1);
            LL_USART_DisableIT_TXFE(USART1);

        } break;
        case COM2: {
            /* USER CODE BEGIN USART2_Init 0 */
            if (pDev->inited == false) {
                pDev->txBuf = (uint8_t *)m_Com2TxBuf;
                pDev->rxBuf = (uint8_t *)m_Com2RxBuf;
                cqueue_create(&pDev->txQueue, pDev->txBuf, COM2_TX_BUF_SIZE, sizeof(uint8_t));
                cqueue_create(&pDev->rxQueue, pDev->rxBuf, COM2_RX_BUF_SIZE, sizeof(uint8_t));
            }
            LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
            /**USART2 GPIO Configuration
            PD5   ------> USART2_TX
            PD6   ------> USART2_RX
            */
            GPIO_InitStruct.Pin        = LL_GPIO_PIN_5 | LL_GPIO_PIN_6;
            GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate  = LL_GPIO_AF_7;
            LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

            NVIC_SetPriority(USART2_IRQn,
                             NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
            NVIC_EnableIRQ(USART2_IRQn);
            USART_InitStruct.BaudRate            = baud;
            USART_InitStruct.DataWidth           = wordLen;
            USART_InitStruct.StopBits            = stopBit;
            USART_InitStruct.Parity              = parity;
            USART_InitStruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
            USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
            USART_InitStruct.OverSampling        = LL_USART_OVERSAMPLING_16;
            LL_USART_Init(USART2, &USART_InitStruct);

            LL_USART_SetTXFIFOThreshold(USART2, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_SetRXFIFOThreshold(USART2, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_EnableFIFO(USART2);

            LL_USART_ConfigAsyncMode(USART2);
            LL_USART_Enable(USART2);
            LL_USART_EnableIT_RXNE(USART2); // 接收中断
            LL_USART_EnableIT_PE(USART2);   // 奇偶校验错误中断
            // LL_USART_EnableIT_TXFE(USART2); //启用TXFIFO Empty中断

        } break;
        case COM3: {
            /* USER CODE BEGIN USART3_Init 0 */
            if (pDev->inited == false) {
                pDev->txBuf = (uint8_t *)m_Com3TxBuf;
                pDev->rxBuf = (uint8_t *)m_Com3RxBuf;
                cqueue_create(&pDev->txQueue, pDev->txBuf, COM3_TX_BUF_SIZE, sizeof(uint8_t));
                cqueue_create(&pDev->rxQueue, pDev->rxBuf, COM3_RX_BUF_SIZE, sizeof(uint8_t));
            }
            
            LL_RCC_SetUSARTClockSource(LL_RCC_USART3_CLKSOURCE_PCLK1);
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);

            /**USART3 GPIO Configuration
            PD8   ------> USART3_TX
            PD9   ------> USART3_RX
            */
            GPIO_InitStruct.Pin        = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
            GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate  = LL_GPIO_AF_7;
            LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

            NVIC_SetPriority(USART3_IRQn,
                             NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 3, 0));
            NVIC_EnableIRQ(USART3_IRQn);
            USART_InitStruct.BaudRate            = baud;
            USART_InitStruct.DataWidth           = wordLen;
            USART_InitStruct.StopBits            = stopBit;
            USART_InitStruct.Parity              = parity;
            USART_InitStruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
            USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
            USART_InitStruct.OverSampling        = LL_USART_OVERSAMPLING_16;
            LL_USART_Init(USART3, &USART_InitStruct);

            LL_USART_SetTXFIFOThreshold(USART3, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_SetRXFIFOThreshold(USART3, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_EnableFIFO(USART3);

            LL_USART_ConfigAsyncMode(USART3);
            LL_USART_Enable(USART3);
            LL_USART_EnableIT_RXNE(USART3); // 接收中断
            LL_USART_EnableIT_PE(USART3);   // 奇偶校验错误中断
            // LL_USART_EnableIT_TXFE(USART3); //启用TXFIFO Empty中断

        } break;
        case COM4: {
            /* USER CODE BEGIN LPUART1_Init 0 */
            if (pDev->inited == false) {
                pDev->txBuf = (uint8_t *)m_Com4TxBuf;
                pDev->rxBuf = (uint8_t *)m_Com4RxBuf;
                cqueue_create(&pDev->txQueue, pDev->txBuf, COM4_TX_BUF_SIZE, sizeof(uint8_t));
                cqueue_create(&pDev->rxQueue, pDev->rxBuf, COM4_RX_BUF_SIZE, sizeof(uint8_t));
            }

            LL_USART_InitTypeDef UART_InitStruct = {0};

            LL_RCC_SetUARTClockSource(LL_RCC_UART4_CLKSOURCE_PCLK1);
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART4);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

            /**UART4 GPIO Configuration
            PC10   ------> UART4_TX
            PC11   ------> UART4_RX
            */
            GPIO_InitStruct.Pin        = LL_GPIO_PIN_10 | LL_GPIO_PIN_11;
            GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate  = LL_GPIO_AF_5;
            LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

            NVIC_SetPriority(UART4_IRQn,
                             NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 3, 0));
            NVIC_EnableIRQ(UART4_IRQn);
            UART_InitStruct.BaudRate            = baud;
            UART_InitStruct.DataWidth           = wordLen;
            UART_InitStruct.StopBits            = stopBit;
            UART_InitStruct.Parity              = parity;
            UART_InitStruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
            UART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
            UART_InitStruct.OverSampling        = LL_USART_OVERSAMPLING_16;
            LL_USART_Init(UART4, &UART_InitStruct);

            LL_USART_SetTXFIFOThreshold(UART4, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_SetRXFIFOThreshold(UART4, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_EnableFIFO(UART4);

            LL_USART_ConfigAsyncMode(UART4);
            LL_USART_Enable(UART4);
            LL_USART_EnableIT_RXNE(UART4); // 接收中断
            LL_USART_EnableIT_PE(UART4);   // 奇偶校验错误中断
            // LL_USART_EnableIT_TXFE(UART4); //启用TXFIFO Empty中断

        } break;
        case COM5: {
            if (pDev->inited == false) {
                pDev->txBuf = (uint8_t *)m_Com5TxBuf;
                pDev->rxBuf = (uint8_t *)m_Com5RxBuf;
                cqueue_create(&pDev->txQueue, pDev->txBuf, COM5_TX_BUF_SIZE, sizeof(uint8_t));
                cqueue_create(&pDev->rxQueue, pDev->rxBuf, COM5_RX_BUF_SIZE, sizeof(uint8_t));
            }

            LL_USART_InitTypeDef UART_InitStruct = {0};

            LL_RCC_SetUARTClockSource(LL_RCC_UART5_CLKSOURCE_PCLK1);
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART5);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);

            /**UART5 GPIO Configuration
            PC12   ------> UART5_TX
            PD2   ------> UART5_RX
            */
            GPIO_InitStruct.Pin        = LL_GPIO_PIN_12;
            GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate  = LL_GPIO_AF_5;
            LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

            GPIO_InitStruct.Pin        = LL_GPIO_PIN_2;
            GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate  = LL_GPIO_AF_5;
            LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

            NVIC_SetPriority(UART5_IRQn,
                             NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 3, 0));
            NVIC_EnableIRQ(UART5_IRQn);
            UART_InitStruct.BaudRate            = baud;
            UART_InitStruct.DataWidth           = wordLen;
            UART_InitStruct.StopBits            = stopBit;
            UART_InitStruct.Parity              = parity;
            UART_InitStruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
            UART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
            UART_InitStruct.OverSampling        = LL_USART_OVERSAMPLING_16;
            LL_USART_Init(UART5, &UART_InitStruct);

            LL_USART_SetTXFIFOThreshold(UART5, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_SetRXFIFOThreshold(UART5, LL_USART_FIFOTHRESHOLD_8_8);
            LL_USART_EnableFIFO(UART5);

            LL_USART_ConfigAsyncMode(UART5);
            LL_USART_Enable(UART5);
            LL_USART_EnableIT_RXNE(UART5); // 接收中断
            LL_USART_EnableIT_PE(UART5);   // 奇偶校验错误中断
            // LL_USART_EnableIT_TXFE(UART5); //启用TXFIFO Empty中断

        } break;

        case COM6: {
            if (pDev->inited == false) {
                pDev->txBuf = (uint8_t *)m_Com6TxBuf;
                pDev->rxBuf = (uint8_t *)m_Com6RxBuf;
                cqueue_create(&pDev->txQueue, pDev->txBuf, COM6_TX_BUF_SIZE, sizeof(uint8_t));
                cqueue_create(&pDev->rxQueue, pDev->rxBuf, COM6_RX_BUF_SIZE, sizeof(uint8_t));
            }

            LL_LPUART_InitTypeDef LPUART_InitStruct = {0};

            /* Peripheral clock enable */
            LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPUART1);

            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

            // 光伏RTU项目的配置
            /**LPUART1 GPIO Configuration
             PC0   ------> LPUART1_RX
            PC1   ------> LPUART1_TX
            */
            GPIO_InitStruct.Pin        = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
            GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
            GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
            GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
            GPIO_InitStruct.Alternate  = LL_GPIO_AF_8;
            LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

            /* LPUART1 interrupt Init */
            NVIC_SetPriority(LPUART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
            NVIC_EnableIRQ(LPUART1_IRQn);

            LPUART_InitStruct.BaudRate            = baud;
            LPUART_InitStruct.DataWidth           = wordLen;
            LPUART_InitStruct.StopBits            = stopBit;
            LPUART_InitStruct.Parity              = parity;
            LPUART_InitStruct.TransferDirection   = LL_LPUART_DIRECTION_TX_RX;
            LPUART_InitStruct.HardwareFlowControl = LL_LPUART_HWCONTROL_NONE;
            LL_LPUART_Init(LPUART1, &LPUART_InitStruct);

            LL_LPUART_SetTXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_7_8);
            LL_LPUART_SetRXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_8_8);
            LL_LPUART_EnableFIFO(LPUART1);

            LL_LPUART_Enable(LPUART1);

            LL_LPUART_EnableIT_RXNE(LPUART1); // 接收中断
            LL_LPUART_EnableIT_PE(LPUART1);   // 奇偶校验错误中断
            LL_LPUART_EnableIT_TXFE(LPUART1); // 启用TXFIFO Empty中断

            /* Polling LPUART1 initialisation */
            while ((!(LL_LPUART_IsActiveFlag_TEACK(LPUART1))) || (!(LL_LPUART_IsActiveFlag_REACK(LPUART1)))) {
            }
            /*有时候初始化完成后TC传输完成标志位始终是0，
            导致先等待TC标志位为RESET的写操作阻塞卡死，所以先清除传输完成标志位*/
            SET_BIT(LPUART1->ISR, USART_ISR_TC);
        } break;
        default:
            break;
    }

    pDev->inited = true;
}

static void Uart_EnableIT_TXE(COMID_t comId)
{
    switch (comId) {
        case COM1:
            LL_USART_EnableIT_TXE(USART1);
            break;
        case COM2:
            LL_USART_EnableIT_TXE(USART2);
            break;
        case COM3:
            LL_USART_EnableIT_TXE(USART3);
            break;
        case COM4:
            LL_USART_EnableIT_TXE(UART4);
            break;
        case COM5:
            LL_USART_EnableIT_TXE(UART5);
            break;
        case COM6:
            LL_LPUART_EnableIT_TXE(LPUART1);
            break;
        default:
            break;
    }
}

/**
 * @brief 串口写操作
 *
 * @param comx 串口号
 * @param writeBuf 存放待写数据缓存区的指针
 * @param uLen 需要写多少个字节
 * @return uint32_t >0-写出去实际字节数，0-未初始化，写失败
 */
uint32_t Uart_Write(COMID_t comId, const uint8_t *writeBuf, uint32_t uLen)
{
    uint32_t uiBytesWritten = 0;

    if (comId >= COM_NUM || writeBuf == NULL || uLen == 0) {
        return uiBytesWritten;
    }

    COM_Dev_t *pDev   = &_gCOMList[comId];
    uint8_t ch        = 0;
    uint32_t push_len = 0;

    if (pDev->inited == false) {
        uiBytesWritten = 0;
        return uiBytesWritten;
    }

    for (int i = 0; i < uLen; i++) {
        ch = writeBuf[i];
        while (true) {
            /* 将新数据填入发送缓冲区 */
            DISABLE_INT();
            push_len = cqueue_in(&_gCOMList[comId].txQueue, &ch, 1);
            ENABLE_INT();
            if (push_len > 0) {
                break;
            } else {
                /* 数据已填满缓冲区 */
                /* 如果发送缓冲区已经满了，则等待缓冲区空 */
                Uart_EnableIT_TXE(comId);
            }
        }
    }

    Uart_EnableIT_TXE(comId);
    return uiBytesWritten;
}

/**
 * @brief 串口读操作
 *
 * @param comx 串口号
 * @param pBuf 存放读取数据的缓存区的指针
 * @param uiLen 本次操作最多能读取的字节数
 * @return uint32_t >0-实际读取的字节数，0-没有数据或者串口不可用
 */
uint32_t Uart_Read(COMID_t comId, unsigned char *pBuf, uint32_t uiLen)
{

    uint32_t uRtn = 0;
    if (comId < COM_NUM) {
        uRtn = cqueue_out(&_gCOMList[comId].rxQueue, pBuf, uiLen);
    }
    return uRtn;
}

/**
 * @brief 获取当前串口接收缓存中收到字节数。
 *
 * @param comId
 * @return uint32_t 当前串口接收缓存中收到字节数。
 */
uint32_t Uart_AvailableBytes(COMID_t comId)
{
    uint32_t uRtn = 0;
    if (comId < COM_NUM) {
        uRtn = cqueue_size(&_gCOMList[comId].rxQueue);
    }
    return uRtn;
}

/**
 * @brief 清空串口接收缓存。
 *
 * @param comId 串口号。
 * @return uint32_t 成功清空的字节数。
 */
uint32_t Uart_EmptyReadBuffer(COMID_t comId)
{
    int uRtn = 0;
    if (comId < COM_NUM) {
        uRtn = cqueue_size(&_gCOMList[comId].rxQueue);
        cqueue_make_empty(&_gCOMList[comId].rxQueue);
    }
    return uRtn;
}

/**
 * @brief 注册Uart_Write写入完成回调函数。需要注意的是两次Uart_Write时间间隔太短可能就不能
 * 对应每个Uart_Write调用产生中断。
 *
 * @param comId 串口号。
 * @param callback 回调函数指针。
 * @param args 回调函数参数。
 * @return uint8_t 成功1，失败0.
 */
uint8_t Uart_SetWriteOverCallback(COMID_t comId, UartWriteOverCallback_t callback, void *args)
{
    uint8_t ret = 0;

    if (comId < COM_NUM) {
        _gCOMList[comId].write_over_callback      = callback;
        _gCOMList[comId].write_over_callback_args = args;
        ret                                       = 1;
    }

    return ret;
}

/**
 * @brief
 *
 * @param comId 串口号。
 * @param callback 回调函数指针。
 * @return uint8_t 成功1，失败0.
 */
uint8_t Uart_RegisterReceiveCharCallback(COMID_t comId, UartReceiveCharCallback_t callback)
{
    uint8_t ret = 0;
    if (comId < COM_NUM) {
        _gCOMList[comId].receive_char_callback = callback;
        ret                                    = 1;
    }
    return ret;
}

/**
 * @brief 取消注册中断中接收字符数据流的函数，这样就能且只能使用Uart_Read读取数据了。
 *
 * @param comId 串口号。
 * @return uint8_t 成功1，失败0.
 */
uint8_t Uart_UnregisterReceiveCharCallback(COMID_t comId)
{
    uint8_t ret = 0;
    if (comId < COM_NUM) {
        _gCOMList[comId].receive_char_callback = NULL;
        ret                                    = 1;
    }
    return ret;
}

void USART_Callback(USART_TypeDef *USARTx, COM_Dev_t *pDev)
{
    uint8_t ch = 0;

    if (LL_USART_IsActiveFlag_RXNE(USARTx) != RESET) // 检测是否接收中断
    {

        ch = LL_USART_ReceiveData8(USARTx); // 读取出来接收到的数据
        // 如果外部单独注册了接收字符数据流的方法，那么就使用外部注册的方法
        if (pDev->receive_char_callback != NULL) {
            pDev->receive_char_callback(ch);
        } else {
            cqueue_enqueue(&pDev->rxQueue, &ch);
        }
    }

    if (LL_USART_IsActiveFlag_ORE(USARTx) != RESET) {
        ch = LL_USART_ReceiveData8(USARTx);
        LL_USART_ClearFlag_ORE(USARTx);
    }
    LL_USART_ClearFlag_FE(USARTx); // Clear Framing Error Flag
    LL_USART_ClearFlag_PE(USARTx); // 奇偶校验错误清除

    // LL_USART_IsEnabledIT_TC是为了防止接收字符时也进入发送中断
    /* 处理发送缓冲区空中断 */
    if (LL_USART_IsEnabledIT_TXE(USARTx) && LL_USART_IsActiveFlag_TXE(USARTx)) {
        if (cqueue_dequeue(&pDev->txQueue, &ch) == 1) {
            LL_USART_TransmitData8(USARTx, ch); // 把数据再从串口发送出去
        } else {
            /* 发送缓冲区的数据已取完时， 禁止发送缓冲区空中断 （注意：此时最后1个数据还未真正发送完毕）*/
            LL_USART_DisableIT_TXE(USARTx);
            /* 使能数据发送完毕中断 */
            LL_USART_EnableIT_TC(USARTx);
        }
    }

    /* 数据bit位全部发送完毕的中断 */
    if (LL_USART_IsEnabledIT_TC(USARTx) && LL_USART_IsActiveFlag_TC(USARTx)) {
        if (cqueue_dequeue(&pDev->txQueue, &ch) == 0) {
            /* 如果发送FIFO的数据全部发送完毕，禁止数据发送完毕中断 */
            LL_USART_DisableIT_TC(USARTx);
            if (pDev->write_over_callback != NULL) {
                pDev->write_over_callback(pDev->write_over_callback_args);
            }
        } else {
            /* 正常情况下，不会进入此分支 */
            /* 如果发送FIFO的数据还未完毕，则从发送FIFO取1个数据写入发送数据寄存器 */
            LL_USART_TransmitData8(USARTx, ch); // 把数据再从串口发送出去
        }
    }
}

/**
 * @brief This function handles USART1 global interrupt.
 */
void USART1_IRQHandler(void)
{
    USART_Callback(USART1, &_gCOMList[COM1]);
}

/**
 * @brief This function handles USART2 global interrupt.
 */
void USART2_IRQHandler(void)
{
    USART_Callback(USART2, &_gCOMList[COM2]);
}

/**
 * @brief This function handles USART3 global interrupt.
 */
void USART3_IRQHandler(void)
{
    USART_Callback(USART3, &_gCOMList[COM3]);
}

/**
 * @brief This function handles UART4 global interrupt.
 */
void UART4_IRQHandler(void)
{
    USART_Callback(LPUART1, &_gCOMList[COM4]);
}

/**
 * @brief This function handles UART5 global interrupt.
 */
void UART5_IRQHandler(void)
{
    USART_Callback(UART5, &_gCOMList[COM5]);
}

/**
 * @brief This function handles LPUART1 global interrupt.
 */
void LPUART1_IRQHandler(void)
{
    USART_Callback(LPUART1, &_gCOMList[COM6]);
}
