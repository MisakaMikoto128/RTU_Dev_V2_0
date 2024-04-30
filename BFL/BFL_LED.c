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

#define LED1_PIN LL_GPIO_PIN_5
#define LED1_Port GPIOE
#define LED1_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOE)

#define LED2_PIN LL_GPIO_PIN_12
#define LED2_Port GPIOB
#define LED2_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB)

#define LED3_PIN LL_GPIO_PIN_13
#define LED3_Port GPIOB
#define LED3_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB)

#define LED4_PIN LL_GPIO_PIN_14
#define LED4_Port GPIOB
#define LED4_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB)

#define LED5_PIN LL_GPIO_PIN_0
#define LED5_Port GPIOD
#define LED5_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD)

#define LED6_PIN LL_GPIO_PIN_1
#define LED6_Port GPIOD
#define LED6_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD)

void BFL_LED_init()
{
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    LED1_Port_CLK_EN();
    GPIO_InitStruct.Pin = LED1_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED1_Port, &GPIO_InitStruct);
    BFL_LED_off(LED1);

    LED2_Port_CLK_EN();
    GPIO_InitStruct.Pin = LED2_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED2_Port, &GPIO_InitStruct);
    BFL_LED_off(LED2);

    LED3_Port_CLK_EN();
    GPIO_InitStruct.Pin = LED3_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED3_Port, &GPIO_InitStruct);
    BFL_LED_off(LED3);

    LED4_Port_CLK_EN();
    GPIO_InitStruct.Pin = LED4_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED4_Port, &GPIO_InitStruct);
    BFL_LED_off(LED4);

    LED5_Port_CLK_EN();
    GPIO_InitStruct.Pin = LED5_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED5_Port, &GPIO_InitStruct);
    BFL_LED_off(LED5);

    LED6_Port_CLK_EN();
    GPIO_InitStruct.Pin = LED6_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(LED6_Port, &GPIO_InitStruct);
    BFL_LED_off(LED6);
}

void BFL_LED_on(LED_t led_id)
{
    switch (led_id)
    {
    case LED1:
        LL_GPIO_SetOutputPin(LED1_Port, LED1_PIN);
        break;
    case LED2:
        LL_GPIO_SetOutputPin(LED2_Port, LED2_PIN);
        break;
    case LED3:
        LL_GPIO_SetOutputPin(LED3_Port, LED3_PIN);
        break;
    case LED4:
        LL_GPIO_SetOutputPin(LED4_Port, LED4_PIN);
        break;
    case LED5:
        LL_GPIO_SetOutputPin(LED5_Port, LED5_PIN);
        break;
    case LED6:
        LL_GPIO_SetOutputPin(LED6_Port, LED6_PIN);
        break;
		default:
			break;
    }
}

void BFL_LED_off(LED_t led_id)
{
    switch (led_id)
    {
    case LED1:
        LL_GPIO_ResetOutputPin(LED1_Port, LED1_PIN);
        break;
    case LED2:
        LL_GPIO_ResetOutputPin(LED2_Port, LED2_PIN);
        break;
    case LED3:
        LL_GPIO_ResetOutputPin(LED3_Port, LED3_PIN);
        break;
    case LED4:
        LL_GPIO_ResetOutputPin(LED4_Port, LED4_PIN);
        break;
    case LED5:
        LL_GPIO_ResetOutputPin(LED5_Port, LED5_PIN);
        break;
    case LED6:
        LL_GPIO_ResetOutputPin(LED6_Port, LED6_PIN);
        break;
				default:
			break;
    }
}

void BFL_LED_toggle(LED_t led_id)
{
    switch (led_id)
    {
    case LED1:
        LL_GPIO_TogglePin(LED1_Port, LED1_PIN);
        break;
    case LED2:
        LL_GPIO_TogglePin(LED2_Port, LED2_PIN);
        break;
    case LED3:
        LL_GPIO_TogglePin(LED3_Port, LED3_PIN);
        break;
    case LED4:
        LL_GPIO_TogglePin(LED4_Port, LED4_PIN);
        break;
    case LED5:
        LL_GPIO_TogglePin(LED5_Port, LED5_PIN);
        break;
    case LED6:
        LL_GPIO_TogglePin(LED6_Port, LED6_PIN);
        break;
				default:
			break;
    }
}