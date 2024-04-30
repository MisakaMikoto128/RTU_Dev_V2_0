#ifndef __HDL_IWDG_H
#define __HDL_IWDG_H

#include "stdint.h"
#include "stm32g4xx_ll_iwdg.h"

extern void HDL_WATCHDOG_Init(uint16_t timeout);
extern void HDL_WATCHDOG_Feed(void);

#endif
