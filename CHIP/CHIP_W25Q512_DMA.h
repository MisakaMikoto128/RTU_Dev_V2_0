/**
* @file CHIP_W25Q512_DMA.h
* @author Liu Yuanlin (liuyuanlins@outlook.com)
* @brief
* @version 0.1
* @date 2024-07-23
* @last modified 2024-07-23
*
* @copyright Copyright (c) 2024 Liu Yuanlin Personal.
*
*/
#ifndef CHIP_W25Q512_DMA_H
#define CHIP_W25Q512_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define CHIP_W25Q512_DMA_ENABLE 1
void CHIP_W25Q512_DMA_Init(void);
#ifdef __cplusplus
}
#endif
#endif //!CHIP_W25Q512_DMA_H
