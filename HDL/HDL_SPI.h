/**
 * @file HDL_SPI.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-01-19
 * @last modified 2023-01-19
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef HDL_SPI_H
#define HDL_SPI_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include "ccommon.h"
typedef enum {
    SPI_1 = 0,
    SPI_NUM,
} SPI_ID_t;

/*
0h (R/W) = Data is output on rising edge and input on falling edge.
When no SPI data is sent, SPICLK is at low level. The data input
and output edges depend on the value of the CLOCK PHASE bit
(SPICTL.3) as follows:
- CLOCK PHASE = 0: Data is output on the rising edge of the
SPICLK signal. Input data is latched on the falling edge of the
SPICLK signal.
- CLOCK PHASE = 1: Data is output one half-cycle before the first
rising edge of the SPICLK signal and on subsequent falling edges
of the SPICLK signal. Input data is latched on the rising edge of
the SPICLK signal.
CPOL as active-low.
*/
#define HDL_SPI_CPOL_LOW LL_SPI_POLARITY_LOW
/*
1h (R/W) = Data is output on falling edge and input on rising edge.
When no SPI data is sent, SPICLK is at high level. The data input
and output edges depend on the value of the CLOCK PHASE bit
(SPICTL.3) as follows:
- CLOCK PHASE = 0: Data is output on the falling edge of the
SPICLK signal. Input data is latched on the rising edge of the
SPICLK signal.
- CLOCK PHASE = 1: Data is output one half-cycle before the first
falling edge of the SPICLK signal and on subsequent rising edges
of the SPICLK signal. Input data is latched on the falling edge of
the SPICLK signal.
CPOL as active-high.
*/
#define HDL_SPI_CPOL_HIGH    LL_SPI_POLARITY_HIGH

#define HDL_SPI_CPHA_1EDGE   LL_SPI_PHASE_1EDGE
#define HDL_SPI_CPHA_2EDGE   LL_SPI_PHASE_2EDGE
#define HDL_SPI_DATA_SIZE_8  (8 - 1)
#define HDL_SPI_DATA_SIZE_16 (16 - 1)

void HDL_SPI_Init(SPI_ID_t spiID, byte_t dataSize, uint32_t CPOL, uint32_t CPHA);
bool HDL_SPI_WriteRead(SPI_ID_t spiID, byte_t *pTxData, byte_t *pRxData, uint16_t size, uint32_t timeout);
uint32_t HDL_SPI_Read(SPI_ID_t spiID, byte_t *pRxData, uint16_t size, uint32_t timeout);
uint32_t HDL_SPI_Write(SPI_ID_t spiID, byte_t *pTxData, uint32_t size, uint32_t timeout);
void HDL_SPI_DeInit(SPI_ID_t spiID);

#ifdef __cplusplus
}
#endif
#endif //! HDL_SPI_H
