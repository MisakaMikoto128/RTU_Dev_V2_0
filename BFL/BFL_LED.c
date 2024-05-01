/**
 * @file BFL_LED.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-12-13
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "BFL_LED.h"
#include "main.h"

#define LED1_PIN           LL_GPIO_PIN_1
#define LED1_Port          GPIOC
#define LED1_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC)

void BFL_LED_Init()
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    LED1_Port_CLK_EN();
    GPIO_InitStruct.Pin        = LED1_PIN;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED1_Port, &GPIO_InitStruct);
    BFL_LED_Off(LED1);
}

void BFL_LED_On(LED_t led_id)
{
    switch (led_id) {
        case LED1:
            LL_GPIO_SetOutputPin(LED1_Port, LED1_PIN);
            break;
        default:
            break;
    }
}

void BFL_LED_Off(LED_t led_id)
{
    switch (led_id) {
        case LED1:
            LL_GPIO_ResetOutputPin(LED1_Port, LED1_PIN);
            break;
        default:
            break;
    }
}

void BFL_LED_Toggle(LED_t led_id)
{
    switch (led_id) {
        case LED1:
            LL_GPIO_TogglePin(LED1_Port, LED1_PIN);
            break;
        default:
            break;
    }
}