/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    user_diskio.c
 * @brief   This file includes a diskio driver skeleton to be completed by the user.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

#ifdef USE_OBSOLETE_USER_CODE_SECTION_0
/*
 * Warning: the user section 0 is no more in use (starting from CubeMx version 4.16.0)
 * To be suppressed in the future.
 * Kept to ensure backward compatibility with previous CubeMx versions when
 * migrating projects.
 * User code previously added there should be copied in the new user sections before
 * the section contents can be deleted.
 */
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */
#endif

/* USER CODE BEGIN DECL */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "ff_gen_drv.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#include "CHIP_W25Q512.h"
#include "./sdcard/bsp_spi_sdcard.h"
#include "log.h"
/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;
/* 为每个设备定义一个物理编号 */
#define ATA       1 // SD卡
#define SPI_FLASH 0 // 预留外部SPI Flash使用
/* USER CODE END DECL */

/* Private function prototypes -----------------------------------------------*/
DSTATUS USER_initialize(BYTE pdrv);
DSTATUS USER_status(BYTE pdrv);
DRESULT USER_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
#if _USE_WRITE == 1
DRESULT USER_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
DRESULT USER_ioctl(BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */

Diskio_drvTypeDef USER_Driver =
    {
        USER_initialize,
        USER_status,
        USER_read,
#if _USE_WRITE
        USER_write,
#endif /* _USE_WRITE == 1 */
#if _USE_IOCTL == 1
        USER_ioctl,
#endif /* _USE_IOCTL == 1 */
};

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  Initializes a Drive
 * @param  pdrv: Physical drive number (0..)
 * @retval DSTATUS: Operation status
 */
DSTATUS USER_initialize(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
)
{
    /* USER CODE BEGIN INIT */
    Stat = STA_NOINIT;

    switch (pdrv) {
        case ATA: /* SD CARD */
            if (SD_Card_Init() == SD_RESPONSE_NO_ERROR) {
                Stat &= ~STA_NOINIT;
            } else {
                Stat |= STA_NOINIT;
            }
            break;
        case SPI_FLASH: /* SPI Flash */
            if (CHIP_W25Q512_Init() == 0) {
                Stat &= ~STA_NOINIT;
            } else {
                Stat |= STA_NOINIT;
            }
            break;
        default:
            Stat = STA_NOINIT;
    }

    return Stat;
    /* USER CODE END INIT */
}

/**
 * @brief  Gets Disk Status
 * @param  pdrv: Physical drive number (0..)
 * @retval DSTATUS: Operation status
 */
DSTATUS USER_status(
    BYTE pdrv /* Physical drive number to identify the drive */
)
{
    /* USER CODE BEGIN STATUS */
    DSTATUS status = 0;
    switch (pdrv) {
        case ATA: /* SD CARD */
            if (SD_Detect() == SD_NOT_PRESENT) {
                status = STA_NODISK;
            } else if (SD_Card_Is_IF_Inited() == false) {
                status = STA_NOINIT;
            }
            break;
        case SPI_FLASH: /* SPI Flash */
            break;
        default:
            status = STA_NOINIT;
    }

    return status;
    /* USER CODE END STATUS */
}

/**
 * @brief  Reads Sector(s)
 * @param  pdrv: Physical drive number (0..)
 * @param  *buff: Data buffer to store read data
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to read (1..128)
 * @retval DRESULT: Operation result
 */
DRESULT USER_read(
    BYTE pdrv,    /* Physical drive nmuber to identify the drive */
    BYTE *buff,   /* Data buffer to store read data */
    DWORD sector, /* Sector address in LBA */
    UINT count    /* Number of sectors to read */
)
{
    /* USER CODE BEGIN READ */

    DRESULT status    = RES_PARERR;
    SD_Error SD_state = SD_RESPONSE_NO_ERROR;

    switch (pdrv) {
        case ATA: /* SD CARD */
            SD_state = SD_ReadMultiBlocks(buff, (uint64_t)sector * SD_BLOCKSIZE, SD_BLOCKSIZE, count);
            if (SD_state != SD_RESPONSE_NO_ERROR) {
                status = RES_ERROR;
                ULOG_INFO("SD Read Error sector %d, count %d", sector, count);
            } else {
                status = RES_OK;
                ULOG_INFO("SD read sector %d, count %d", sector, count);
            }
            break;
        case SPI_FLASH: /* SPI Flash */
            for (UINT i = 0; i < count; i++) {
                CHIP_W25Q512_read_one_sector(sector + i, buff + i * W25Q512_SECTOR_SIZE);
            }
            status = RES_OK;
            break;
        default:
            status = RES_PARERR;
            break;
    }

    /* USER CODE END READ */
    return status;
}

#include "log.h"
/**
 * @brief  Writes Sector(s)
 * @param  pdrv: Physical drive number (0..)
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */
#if _USE_WRITE == 1
DRESULT USER_write(
    BYTE pdrv,        /* Physical drive nmuber to identify the drive */
    const BYTE *buff, /* Data to be written */
    DWORD sector,     /* Sector address in LBA */
    UINT count        /* Number of sectors to write */
)
{
    /* USER CODE BEGIN WRITE */
    /* USER CODE HERE */
    DRESULT status    = RES_PARERR;
    SD_Error SD_state = SD_RESPONSE_NO_ERROR;
    if (!count) {
        return RES_PARERR; /* Check parameter */
    }
    switch (pdrv) {
        case ATA: /* SD CARD */

            SD_state = SD_WriteMultiBlocks((uint8_t *)buff, (uint64_t)sector * SD_BLOCKSIZE, SD_BLOCKSIZE, count);
            if (SD_state == SD_RESPONSE_NO_ERROR) {
                status = RES_OK;
                ULOG_INFO("[FatFS] SD write sector %d, count %d", sector, count);
            } else {
                ULOG_INFO("SD Write Error sector %d, count %d", sector, count);
                status = RES_ERROR;
            }
            break;

        case SPI_FLASH:
            for (UINT i = 0; i < count; i++) {
                w25q512_write_one_sector(sector + i, (uint8_t *)buff + i * W25Q512_SECTOR_SIZE);
                ULOG_INFO("[FatFS] Flash write sector %d", sector + i);
            }
            status = RES_OK;
            break;

        default:
            status = RES_PARERR;
            break;
    }
    /* USER CODE END WRITE */
    return status;
}
#endif /* _USE_WRITE == 1 */

/**
 * @brief  I/O control operation
 * @param  pdrv: Physical drive number (0..)
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
#if _USE_IOCTL == 1
DRESULT USER_ioctl(
    BYTE pdrv, /* Physical drive nmuber (0..) */
    BYTE cmd,  /* Control code */
    void *buff /* Buffer to send/receive control data */
)
{
    /* USER CODE BEGIN IOCTL */
    DRESULT res = RES_PARERR;

    switch (cmd) {
        case CTRL_SYNC:
            res = RES_OK;
            break;
        case GET_SECTOR_SIZE:
            if (pdrv == ATA) {
                *(WORD *)buff = SD_BLOCKSIZE;
                res           = RES_OK;
            } else if (pdrv == SPI_FLASH) {
                *(WORD *)buff = (WORD)W25Q512_SECTOR_SIZE;
                res           = RES_OK;
            }
            break;
        case GET_BLOCK_SIZE:
            // Get erase block size in unit of sector (DWORD)
            if (pdrv == ATA || pdrv == SPI_FLASH) {
                *(DWORD *)buff = 1;
                res            = RES_OK;
            }
            break;
        case GET_SECTOR_COUNT:
            if (pdrv == ATA) {
                *(DWORD *)buff = SDCardInfo.CardCapacity / SDCardInfo.CardBlockSize;
                res            = RES_OK;
            } else if (pdrv == SPI_FLASH) {
                *(DWORD *)buff = W25Q512_SECTOR_COUNT;
                res            = RES_OK;
            }
            break;
        default:
            break;
    }

    /* USER CODE END IOCTL */
    return res;
}
#endif /* _USE_IOCTL == 1 */
