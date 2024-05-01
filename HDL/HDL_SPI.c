/**
 * @file HDL_SPI.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-01-19
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "HDL_SPI.h"
#include "HDL_CPU_Time.h"
#include "main.h"

/**
 * @brief SPI初始化。默认为全双工主机,MSB First。通信频率会尽量接近SPI外设允许的最大频率。软件片选。
 *
 * @param spiID SPI设备ID。
 * @param dataSize 数SPI数据读写据位宽8或者16bit。4-16bit。默认8bit。
 * @param CPOL Specifies the serial clock steady state. HDL_SPI_CPOL_LOW: Low level, HDL_SPI_CPOL_HIGH: High level.
 * @param CPHA Specifies the clock active edge for the bit capture. HDL_SPI_CPHA_1EDGE: The first clock transition is the first data capture edge, HDL_SPI_CPHA_2EDGE: The second clock transition is the first data capture edge.
 */
void HDL_SPI_Init(SPI_ID_t spiID, uint8_t dataSize, uint32_t CPOL, uint32_t CPHA)
{
    /* USER CODE BEGIN SPI2_Init 0 */

    /* USER CODE END SPI2_Init 0 */

    LL_SPI_InitTypeDef SPI_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    /**SPI2 GPIO Configuration
    PB12   ------> SPI2_NSS
    PB13   ------> SPI2_SCK
    PB14   ------> SPI2_MISO
    PB15   ------> SPI2_MOSI
    */
    // GPIO_InitStruct.Pin        = LL_GPIO_PIN_12;
    // GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    // GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    // GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    // GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    // GPIO_InitStruct.Alternate  = LL_GPIO_AF_5;
    // LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_13;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate  = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_14;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate  = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_15;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate  = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USER CODE BEGIN SPI2_Init 1 */

    uint32_t DataWidth = LL_SPI_DATAWIDTH_8BIT;
    if (dataSize >= 4 && dataSize <= 16) {
        const uint32_t dataWidthTable[] = {LL_SPI_DATAWIDTH_4BIT, LL_SPI_DATAWIDTH_5BIT, LL_SPI_DATAWIDTH_6BIT, LL_SPI_DATAWIDTH_7BIT, LL_SPI_DATAWIDTH_8BIT, LL_SPI_DATAWIDTH_9BIT, LL_SPI_DATAWIDTH_10BIT, LL_SPI_DATAWIDTH_11BIT, LL_SPI_DATAWIDTH_12BIT, LL_SPI_DATAWIDTH_13BIT, LL_SPI_DATAWIDTH_14BIT, LL_SPI_DATAWIDTH_15BIT, LL_SPI_DATAWIDTH_16BIT};
        DataWidth                       = dataWidthTable[dataSize - 4];
    }

    /* USER CODE END SPI2_Init 1 */
    SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
    SPI_InitStruct.Mode              = LL_SPI_MODE_MASTER;
    SPI_InitStruct.DataWidth         = DataWidth;
    SPI_InitStruct.ClockPolarity     = CPOL;
    SPI_InitStruct.ClockPhase        = CPHA;
    SPI_InitStruct.NSS               = LL_SPI_NSS_SOFT;
    SPI_InitStruct.BaudRate          = LL_SPI_BAUDRATEPRESCALER_DIV4;
    SPI_InitStruct.BitOrder          = LL_SPI_MSB_FIRST;
    SPI_InitStruct.CRCCalculation    = LL_SPI_CRCCALCULATION_DISABLE;
    SPI_InitStruct.CRCPoly           = 7;
    LL_SPI_Init(SPI2, &SPI_InitStruct);
    LL_SPI_SetStandard(SPI2, LL_SPI_PROTOCOL_MOTOROLA);
    LL_SPI_DisableNSSPulseMgt(SPI2);
    /* USER CODE BEGIN SPI2_Init 2 */
    LL_SPI_Enable(SPI2);
    /* USER CODE END SPI2_Init 2 */
}

/**
 * @brief SPI读写。阻塞式读写方法。不能再中断中调用。默认超时时间为10ms。
 *
 * @param spiID SPI设备ID。
 * @param pTxData 发送数据缓冲区。
 * @param pRxData 接收数据缓冲区。
 * @param size 接收数据的长度。
 * @return true 读写成功。
 * @return false 读写失败。
 */
bool HDL_SPI_WriteRead(SPI_ID_t spiID, byte_t *pTxData, byte_t *pRxData, uint16_t size, uint32_t timeout)
{
    if (pTxData == NULL || pRxData == NULL || size == 0) {
        return false;
    }

    uint32_t startMoment = HDL_CPU_Time_GetTick();

    while (size--) {
        while (LL_SPI_IsActiveFlag_TXE(SPI2) == RESET) {
            if (HDL_CPU_Time_GetTick() - startMoment > timeout) {
                return false;
            }
        }
        LL_SPI_TransmitData8(SPI2, *pTxData++);
        while (!LL_SPI_IsActiveFlag_RXNE(SPI2)) {
            if (HDL_CPU_Time_GetTick() - startMoment > timeout) {
                return false;
            }
        }
        *pRxData++ = LL_SPI_ReceiveData8(SPI2);
    }
    return true;
}

/**
 * @brief SPI写。
 *
 * @param spiID SPI设备ID。
 * @param pTxData 发送数据缓冲区。
 * @param size 数据长度。
 * @return true 写成功。
 * @return false 写失败。
 */
uint32_t HDL_SPI_Write(SPI_ID_t spiID, byte_t *pTxData, uint32_t size, uint32_t timeout)
{
    if (pTxData == NULL || size == 0) {
        return false;
    }
    uint32_t startMoment = 0;
    uint32_t ret         = 0;

    startMoment = HDL_CPU_Time_GetTick();
    while (size--) {
        while (!LL_SPI_IsActiveFlag_TXE(SPI2)) {
            if (HDL_CPU_Time_GetTick() - startMoment > timeout) {
                return false;
            }
        }
        LL_SPI_TransmitData8(SPI2, *pTxData++);
    }
    return true;
}

/**
 * @brief SPI读。
 *
 * @param spiID SPI设备ID。
 * @param pRxData 接收数据缓冲区。
 * @param size 要读取的数据长度。
 * @return uint32_t 实际读取的数据长度。
 */
uint32_t HDL_SPI_Read(SPI_ID_t spiID, byte_t *pRxData, uint16_t size, uint32_t timeout)
{
    if (pRxData == NULL || size == 0) {
        return false;
    }
    uint32_t startMoment = 0;
    uint32_t ret         = 0;
    startMoment          = HDL_CPU_Time_GetTick();

    while (size--) {
        while (!LL_SPI_IsActiveFlag_RXNE(SPI2)) {
            if (HDL_CPU_Time_GetTick() - startMoment > timeout) {
                return false;
            }
        }
        *pRxData++ = LL_SPI_ReceiveData8(SPI2);
    }
    return true;
}

/**
 * @brief SPI去初始化。
 *
 * @param spiID SPI设备ID。
 */
void HDL_SPI_DeInit(SPI_ID_t spiID)
{
    LL_SPI_Disable(SPI2);
    LL_SPI_DeInit(SPI2);
    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_SPI2);
}