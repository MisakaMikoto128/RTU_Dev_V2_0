/**
 * @file CHIP_EC800M.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-09-16
 * @last modified 2023-09-16
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#include "CHIP_EC800M.h"
#include "HDL_CPU_Time.h"
#define __CHIP_EC800M_DEBUG 0

/**
 * @brief 用于已经链接到板子的模块使用串口直接与模块通信，用于调试
 *
 */
void __CHIP_EC800M_DEBUG_Init()
{
    Uart_Init(CHIP_EC800M_COM, 115200UL, LL_LPUART_DATAWIDTH_8B, LL_LPUART_STOPBITS_1, LL_LPUART_PARITY_NONE);
}

void CHIP_EC800M_Init()
{
    LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOE);
    /**/
    GPIO_InitStruct.Pin        = RST_4G_Pin | PEN_4G_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = DTR_4G_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(DTR_4G_GPIO_Port, &GPIO_InitStruct);

    /**/
    GPIO_InitStruct.Pin        = RI_4G_Pin;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(RI_4G_GPIO_Port, &GPIO_InitStruct);

    // /**/
    // LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE12);

    // /**/
    // EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_12;
    // EXTI_InitStruct.Line_32_63 = LL_EXTI_LINE_NONE;
    // EXTI_InitStruct.LineCommand = ENABLE;
    // EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
    // EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
    // LL_EXTI_Init(&EXTI_InitStruct);

    // /* EXTI interrupt init*/
    // NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    // NVIC_EnableIRQ(EXTI15_10_IRQn);

#if __CHIP_EC800M_DEBUG == 1
    __CHIP_EC800M_DEBUG_Init();
#else
    Uart_Init(CHIP_EC800M_COM, 115200, LL_USART_DATAWIDTH_8B, LL_USART_STOPBITS_1, LL_USART_PARITY_NONE);
#endif

    CHIP_EC800M_PowerOn();
    CHIP_EC800M_WakeupOff();
    // TODO:修改为osDelay
    HDL_CPU_Time_DelayMs(33);
    CHIP_EC800M_ResetOn();
}

/**
 * @brief This function handles EXTI line[15:10] interrupts.
 */
void EXTI15_10_IRQHandler(void)
{
    /* USER CODE BEGIN EXTI15_10_IRQn 0 */

    /* USER CODE END EXTI15_10_IRQn 0 */
    if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_12) != RESET) {
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_12);
        /* USER CODE BEGIN LL_EXTI_LINE_12 */

        /* USER CODE END LL_EXTI_LINE_12 */
    }
    /* USER CODE BEGIN EXTI15_10_IRQn 1 */

    /* USER CODE END EXTI15_10_IRQn 1 */
}

void CHIP_EC800M_PowerOn()
{
    LL_GPIO_SetOutputPin(PEN_4G_GPIO_Port, PEN_4G_Pin);
}

/**
 * @brief 切断模块供电。
 *
 */
void CHIP_EC800M_PowerOff()
{
    LL_GPIO_ResetOutputPin(PEN_4G_GPIO_Port, PEN_4G_Pin);
}

/**
 * @brief DTR用来唤醒睡眠的模块，置为高电平是可以唤醒。置为低电平不产生作用。
 *
 */
void CHIP_EC800M_WakeupOff()
{
    LL_GPIO_ResetOutputPin(DTR_4G_GPIO_Port, DTR_4G_Pin);
}

/**
 * @brief DTR用来唤醒睡眠的模块，置为高电平是可以唤醒。置为低电平不产生作用。
 *
 */
void CHIP_EC800M_WakeupOn()
{
    LL_GPIO_SetOutputPin(DTR_4G_GPIO_Port, DTR_4G_Pin);
}

/**
 * @brief 用来控制REST_N引脚的电平。置为高电平。
 *
 */
void CHIP_EC800M_ResetOn()
{
    LL_GPIO_SetOutputPin(RST_4G_GPIO_Port, RST_4G_Pin);
}

/**
 * @brief 用来控制REST_N引脚的电平。置为低电平。
 *
 */
void CHIP_EC800M_ResetOff()
{
    LL_GPIO_ResetOutputPin(RST_4G_GPIO_Port, RST_4G_Pin);
}
