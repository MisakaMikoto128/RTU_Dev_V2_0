/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : usbd_storage_if.c
 * @version        : v3.0_Cube
 * @brief          : Memory management layer.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_storage_if.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
 * @brief Usb device.
 * @{
 */

/** @defgroup USBD_STORAGE
 * @brief Usb mass storage device module
 * @{
 */

/** @defgroup USBD_STORAGE_Private_TypesDefinitions
 * @brief Private types.
 * @{
 */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Defines
 * @brief Private defines.
 * @{
 */

#define STORAGE_LUN_NBR 1
// 定义存储设备的 LUN
#define LUN_SPI_FLASH 0
#define LUN_SD_CARD   1
/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Macros
 * @brief Private macros.
 * @{
 */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_Variables
 * @brief Private variables.
 * @{
 */

/* USER CODE BEGIN INQUIRY_DATA_FS */
/** USB Mass storage Standard Inquiry Data. */
const int8_t STORAGE_Inquirydata_FS[] = {
    /* 36 */

    /* LUN 0 */
    0x00,
    0x80,
    0x02,
    0x02,
    (STANDARD_INQUIRY_DATA_LEN - 5),
    0x00,
    0x00,
    0x00,
    'S', 'T', 'M', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
    'P', 'r', 'o', 'd', 'u', 'c', 't', ' ', /* Product      : 16 Bytes */
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    '0', '.', '0', '1', /* Version      : 4 Bytes */
    /* LUN 0 */
    0x00,
    0x80,
    0x02,
    0x02,
    (STANDARD_INQUIRY_DATA_LEN - 5),
    0x00,
    0x00,
    0x00,
    'S', 'T', 'M', ' ', ' ', ' ', ' ', ' ', /* Manufacturer : 8 bytes */
    'P', 'r', 'o', 'd', 'u', 'c', 't', ' ', /* Product      : 16 Bytes */
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    '0', '.', '0', '1', /* Version      : 4 Bytes */
};
/* USER CODE END INQUIRY_DATA_FS */

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Exported_Variables
 * @brief Public variables.
 * @{
 */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
 * @}
 */

/** @defgroup USBD_STORAGE_Private_FunctionPrototypes
 * @brief Private functions declaration.
 * @{
 */

static int8_t STORAGE_Init_FS(uint8_t lun);
static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_FS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun);
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_FS(void);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */
#include "CHIP_W25Q512.h"
#include "./sdcard/bsp_spi_sdcard.h"
#include "log.h"
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
 * @}
 */

USBD_StorageTypeDef USBD_Storage_Interface_fops_FS =
    {
        STORAGE_Init_FS,
        STORAGE_GetCapacity_FS,
        STORAGE_IsReady_FS,
        STORAGE_IsWriteProtected_FS,
        STORAGE_Read_FS,
        STORAGE_Write_FS,
        STORAGE_GetMaxLun_FS,
        (int8_t *)STORAGE_Inquirydata_FS};

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  Initializes over USB FS IP
 * @param  lun:
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_Init_FS(uint8_t lun)
{
    /* USER CODE BEGIN 2 */
    int8_t ret = USBD_OK;

    // switch (lun) {

    //     case LUN_SPI_FLASH: // LUN 1: SPI 闪存
    //         if (CHIP_W25Q512_Init() != 0) {
    //             ret = USBD_FAIL;
    //         }
    //         break;

    //     case LUN_SD_CARD: // LUN 0: SD 卡
    //         if (SD_Card_Init() != SD_RESPONSE_NO_ERROR) {
    //             ret = USBD_FAIL;
    //         }
    //         break;

    //     default: // 未知的 LUN
    //         ret = USBD_FAIL;
    //         break;
    // }

    return ret;
    /* USER CODE END 2 */
}

/**
 * @brief  .
 * @param  lun: .
 * @param  block_num: .
 * @param  block_size: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    /* USER CODE BEGIN 3 */
    int8_t ret = USBD_OK;
    switch (lun) {
        case LUN_SPI_FLASH: // LUN 1: SPI 闪存
            *block_num  = CHIP_W25Q512_GetSectorNum();
            *block_size = CHIP_W25Q512_GetSectorSize();
            break;

        case LUN_SD_CARD: // LUN 0: SD 卡
            *block_num  = 10000;
            *block_size = SD_BLOCKSIZE;
            break;

        default: // 未知的 LUN
            ret = USBD_FAIL;
            break;
    }
    ULOG_INFO("lun=%d, block_num=%d, block_size=%d", lun, *block_num, *block_size);
    return ret;
    /* USER CODE END 3 */
}

/**
 * @brief  .
 * @param  lun: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_IsReady_FS(uint8_t lun)
{
    /* USER CODE BEGIN 4 */
    return (USBD_OK);
    /* USER CODE END 4 */
}

/**
 * @brief  .
 * @param  lun: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_IsWriteProtected_FS(uint8_t lun)
{
    /* USER CODE BEGIN 5 */
    return (USBD_OK);
    /* USER CODE END 5 */
}

/**
 * @brief  .
 * @param  lun: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    /* USER CODE BEGIN 6 */
    int8_t ret = USBD_OK;
    switch (lun) {
        case LUN_SPI_FLASH: // LUN 1: SPI 闪存
            for (uint32_t i = 0; i < blk_len; i++) {
                if (CHIP_W25Q512_read_one_sector(blk_addr + i, buf + i * W25Q512_SECTOR_SIZE) == 0) {
                    ret = USBD_FAIL;
                    break;
                }
            }
            break;

        case LUN_SD_CARD: // LUN 0: SD 卡
            if (SD_ReadMultiBlocks(buf, (uint64_t)blk_addr * SD_BLOCKSIZE, SD_BLOCKSIZE, blk_len) != SD_RESPONSE_NO_ERROR) {
                ret = USBD_FAIL;
            }
            break;

        default: // 未知的 LUN
            // ret = USBD_FAIL;
            break;
    }

    return ret;
    /* USER CODE END 6 */
}

/**
 * @brief  .
 * @param  lun: .
 * @retval USBD_OK if all operations are OK else USBD_FAIL
 */
int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    /* USER CODE BEGIN 7 */

    int8_t ret = USBD_OK;

    switch (lun) {

        case LUN_SPI_FLASH: // LUN 1: SPI 闪存
            for (uint32_t i = 0; i < blk_len; i++) {
                if (w25q512_write_one_sector(blk_addr + i, buf + i * W25Q512_SECTOR_SIZE) != 0) {
                    ret = USBD_FAIL;
                    break;
                }
            }
            break;

        case LUN_SD_CARD: // LUN 0: SD 卡
            if (SD_WriteMultiBlocks((uint8_t *)buf, (uint64_t)blk_addr * SD_BLOCKSIZE, SD_BLOCKSIZE, blk_len) != SD_RESPONSE_NO_ERROR) {
                ret = USBD_FAIL;
            }
            break;

        default: // 未知的 LUN
            // ret = USBD_FAIL;
            break;
    }

    return ret;
    /* USER CODE END 7 */
}

/**
 * @brief  .
 * @param  None
 * @retval .
 */
int8_t STORAGE_GetMaxLun_FS(void)
{
    /* USER CODE BEGIN 8 */
    return (STORAGE_LUN_NBR - 1);
    /* USER CODE END 8 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
 * @}
 */

/**
 * @}
 */
