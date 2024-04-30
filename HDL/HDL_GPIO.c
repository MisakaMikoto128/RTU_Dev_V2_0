/**
 * @file HDL_GPIO.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-01-16
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "HDL_GPIO.h"

void HDL_GPIO_EnablePeripheralCLK(GPIO_TypeDef *GPIOx)
{
    switch ((uint32_t)GPIOx) {
#ifdef GPIOA
        case (uint32_t)GPIOA:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
            break;
#endif
#ifdef GPIOB
        case (uint32_t)GPIOB:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
            break;
#endif
#ifdef GPIOC
        case (uint32_t)GPIOC:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
            break;
#endif
#ifdef GPIOD
        case (uint32_t)GPIOD:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD);
            break;
#endif
#ifdef GPIOE
        case (uint32_t)GPIOE:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOE);
            break;
#endif
#ifdef GPIOF
        case (uint32_t)GPIOF:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOF);
            break;
#endif
#ifdef GPIOG
        case (uint32_t)GPIOG:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOG);
            break;
#endif
#ifdef GPIOH
        case (uint32_t)GPIOH:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOH);
            break;
#endif
#ifdef GPIOI
        case (uint32_t)GPIOI:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOI);
            break;
#endif
#ifdef GPIOJ
        case (uint32_t)GPIOJ:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOJ);
            break;
#endif
#ifdef GPIOK
        case (uint32_t)GPIOK:
            LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOK);
            break;
#endif
        default:
            break;
    }
}
