/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file   app_fatfs.c
 * @brief  Code for fatfs applications
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

/* Includes ------------------------------------------------------------------*/
#include "app_fatfs.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "datetime.h"
#include "log.h"
#include <string.h>
#include "mtime.h"
#include <math.h>
#include <stdlib.h>
#include "crc.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    APPLICATION_IDLE = 0,
    APPLICATION_INIT,
    APPLICATION_RUNNING,
    APPLICATION_SD_UNPLUGGED,
} FS_FileOperationsTypeDef;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
static FATFS fs; /* File system object for USER logical drive */
static FATFS fs_sd;
static uint8_t work_buff[_MAX_SS];
FIL USERFile1;    /* File  object for USER */
FIL USERFile2;    /* File  object for USER */
char USERPath[4]; /* USER logical drive path */
/* USER CODE BEGIN PV */
FS_FileOperationsTypeDef Appli_state = APPLICATION_IDLE;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
 * @brief  FatFs initialization
 * @param  None
 * @retval Initialization result
 */
int32_t MX_FATFS_Init(void)
{
    /*## FatFS: Link the disk I/O driver(s)  ###########################*/

    if (FATFS_LinkDriver(&USER_Driver, USERPath) != 0)
    /* USER CODE BEGIN FATFS_Init */
    {
        return APP_ERROR;
    } else {
        Appli_state = APPLICATION_INIT;
        return APP_OK;
    }
    /* USER CODE END FATFS_Init */
}

/**
 * @brief  FatFs application main process
 * @param  None
 * @retval Process result
 */
int32_t MX_FATFS_Process(void)
{
    /* USER CODE BEGIN FATFS_Process */
    int32_t process_res = APP_OK;

    return process_res;
    /* USER CODE END FATFS_Process */
}

/**
 * @brief  Gets Time from RTC (generated when FS_NORTC==0; see ff.c)
 * @param  None
 * @retval Time in DWORD
 */
DWORD get_fattime(void)
{
    /* USER CODE BEGIN get_fattime */
    mtime_t datetime;
    datetime_get_localtime(&datetime);

    DWORD fat_time = 0;
    fat_time |= ((DWORD)datetime.nYear - 1980) << 25;
    fat_time |= (DWORD)datetime.nMonth << 21;
    fat_time |= (DWORD)datetime.nDay << 16;
    fat_time |= (DWORD)datetime.nHour << 11;
    fat_time |= (DWORD)datetime.nMin << 5;
    fat_time |= (DWORD)datetime.nSec >> 1;
    return fat_time;
    /* USER CODE END get_fattime */
}

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN Application */

void fatfs_register()
{
    FRESULT g_res;
    g_res = f_mount(&fs, "0:", 1);
    if (g_res == FR_NO_FILESYSTEM) {
        ULOG_INFO("[FatFs] No file systems.");
        g_res = f_mkfs("0:", FM_ANY, 0, work_buff, sizeof(work_buff));
        if (g_res == FR_OK) {
            ULOG_INFO("[FatFs] f_mkfs ok");
            /* 格式化后，先取消挂载 */
            f_mount(NULL, "1:", 1);
            /* 重新挂载 */
            f_mount(&fs_sd, "1:", 1);
        } else {
            ULOG_INFO("[FatFs] f_mkfs failed err code = %d", g_res);
        }
    } else if (g_res != FR_OK) {
        ULOG_INFO("[FatFs] f_mount failed err code = %d", g_res);
    } else {
        ULOG_INFO("[FatFs] flash have file systems");
    }
}

void fatfs_unregister()
{
    FRESULT g_res;
    g_res = f_mount(NULL, "0:", 1);
    if (g_res != FR_OK) {
        ULOG_INFO("[FatFs] file systems unregister failed err code = %d", g_res);
    } else {
        ULOG_INFO("[FatFs] file systems unregister ok");
    }
}

void fatfs_rw_test()
{
    FRESULT g_res;

    uint8_t w_buf[]    = "AAAAAAAAAAAAAAAA";
    uint8_t r_buf[200] = {0};
    UINT w_buf_len     = 0;
    UINT r_buf_len     = 0;
    g_res              = f_open(&USERFile1, "0:2020.txt", FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
    if (g_res != FR_OK) {
        ULOG_INFO("[FatFs] open 2020.txt failed err code = %d", g_res);
    } else {
        ULOG_INFO("[FatFs] open 2020.txt  ok");
    }
    g_res = f_write(&USERFile1, w_buf, sizeof(w_buf), &w_buf_len);
    if (g_res != FR_OK) {
        ULOG_INFO("[FatFs] write 2020.txt failed  err code = %d", g_res);
    } else {
        ULOG_INFO("[FatFs] write 2020.txt  ok   w_buf_len = %d", w_buf_len);
        f_lseek(&USERFile1, 0);
        g_res = f_read(&USERFile1, r_buf, f_size(&USERFile1), &r_buf_len);
        if (g_res != FR_OK) {
            ULOG_INFO("[FatFs] read 2020.txt failed g_res = %d", g_res);
        } else {
            ULOG_INFO("[FatFs] read 2020.txt  ok   r_buf_len = %d", r_buf_len);
        }
    }
    f_close(&USERFile1);

    // 打开文件输出文件内容
    g_res = f_open(&USERFile1, "0:2020.txt", FA_OPEN_ALWAYS | FA_WRITE | FA_READ);
    if (g_res != FR_OK) {
        ULOG_INFO("[FatFs] open 2020.txt failed err code = %d", g_res);
    } else {
        ULOG_INFO("[FatFs] open 2020.txt  ok");
    }

    f_lseek(&USERFile1, 0);
    g_res = f_read(&USERFile1, r_buf, f_size(&USERFile1), &r_buf_len);
    if (g_res != FR_OK) {
        ULOG_INFO("[FatFs] read 2020.txt failed g_res = %d", g_res);
    } else {
        ULOG_INFO("[FatFs] read 2020.txt  ok   r_buf_len = %d", r_buf_len);
        ULOG_INFO("[FatFs] read 2020.txt  ok   r_buf = %s", r_buf);
    }
    f_close(&USERFile1);
}

#include "ymodem.h"
int CRC32_File(FIL *pFile, uint32_t *crcRes);
// YmodemSession_t ymodem_session;
// size_t ymodem_read(uint8_t *buf, size_t size)
// {
//   return Uart_Read(COM1, buf, size);
// }

// size_t ymodem_write(const uint8_t *buf, size_t size)
// {
//   return Uart_Write(COM1, (uint8_t *)buf, size);
// }

/*-----------------------------------------------------------------------*/
/* Delete a File/Directory                                               */
/*-----------------------------------------------------------------------*/
bool fatfs_remove(const char *path)
{
    if (f_unlink(path) == FR_OK) {
        return true;
    } else {
        return false;
    }
}

bool fatfs_rename(const char *oldpath, const char *newpath)
{
    if (f_rename(oldpath, newpath) == FR_OK) {
        return true;
    } else {
        return false;
    }
}

bool fatfs_is_file_exist(const char *path)
{
    if (f_stat(path, NULL) == FR_OK) {
        return true;
    } else {
        return false;
    }
}

void W25Q512_Flash_FatFs_Init()
{
    MX_FATFS_Init();
    fatfs_register();
}

void SD_Test();
bool sd_card_is_mounted = false;
bool SD_Card_Fs_IsMounted()
{
    return sd_card_is_mounted;
}

void SD_Card_FatFs_Init()
{
    if (FATFS_LinkDriverEx(&USER_Driver, USERPath, 1) != 0)
    /* USER CODE BEGIN FATFS_Init */
    {
        ULOG_INFO("[FatFs] FATFS_LinkDriver failed");
    } else {
        Appli_state = APPLICATION_INIT;
        ULOG_INFO("[FatFs] FATFS_LinkDriver ok");
    }

    FRESULT g_res;
    g_res = f_mount(&fs_sd, "1:", 1);
    if (g_res == FR_NO_FILESYSTEM) {
        ULOG_INFO("[FatFs] No file systems.");
        FRESULT res = f_mkfs("1:", FM_ANY, 0, work_buff, sizeof(work_buff));
        if (res == FR_OK) {
            ULOG_INFO("[FatFs] f_mkfs ok");
            /* 重新挂载 */
            g_res = f_mount(&fs_sd, "1:", 1);
            sd_card_is_mounted = true;
        } else {
            ULOG_INFO("[FatFs] f_mkfs failed err code = %d", g_res);
        }
    } else if (g_res != FR_OK) {
        ULOG_INFO("[FatFs] f_mount failed err code = %d", g_res);
    } else {
        ULOG_INFO("[FatFs] flash have file systems");
        sd_card_is_mounted = true;
        // 打印文件系统信息
        FATFS *fs;
        DWORD fre_clust, fre_sect, tot_sect;
        uint64_t fre_space, tot_space;

        f_getfree("1:", &fre_clust, &fs);
        /* Get total sectors and free sectors */
        tot_sect  = (fs->n_fatent - 2) * fs->csize;
        fre_sect  = fre_clust * fs->csize;
        fre_space = (uint64_t)fre_sect * fs->ssize;
        tot_space = (uint64_t)tot_sect * fs->ssize;
        ULOG_INFO("[FatFs] SD Card file system information:");
        ULOG_INFO("[FatFs] Total sectors: %u", tot_sect);
        ULOG_INFO("[FatFs] Free sectors: %u", fre_sect);
        ULOG_INFO("[FatFs] Sector size: %u", fs->ssize);
        ULOG_INFO("[FatFs] Cluster size: %u byte", fs->csize * fs->ssize);
        /* Print the free space (assuming 512 bytes/sector) */
        ULOG_INFO("[FatFs] SD Card free space: %.2f GB", fre_space / (1024 * 1024 * 1024.0f));
        /* Print the total space (assuming 512 bytes/sector) */
        ULOG_INFO("[FatFs] SD Card total space: %.2f GB", tot_space / (1024 * 1024 * 1024.0f));
        /* Print disk format */
        ULOG_INFO("[FatFs] SD Card format: %s", fs->fs_type == FS_FAT12 ? "FAT12" : fs->fs_type == FS_FAT16 ? "FAT16"
                                                                                : fs->fs_type == FS_FAT32   ? "FAT32"
                                                                                                           : "Unknown");
    }
}

void SD_Card_FatFs_DeInit()
{
    FRESULT g_res;
    g_res = f_mount(NULL, "1:", 1);
    if (g_res != FR_OK) {
        ULOG_INFO("[FatFs] file systems unregister failed err code = %d", g_res);
    } else {
        ULOG_INFO("[FatFs] file systems unregister ok");
    }
    FATFS_UnLinkDriver(USERPath);
    sd_card_is_mounted = false;
}

void FatFs_Init()
{
    W25Q512_Flash_FatFs_Init();
    SD_Card_FatFs_Init();

    // 列出根目录下所有文件和文件夹
    FRESULT res;
    DIR dir;
    FILINFO fno;
    char *fn; /* This function assumes non-Unicode configuration */
    char path[100];
    strcpy(path, "0:");
    res = f_opendir(&dir, path); /* Open the directory */

    if (res == FR_OK) {
        while (1) {
            res = f_readdir(&dir, &fno); /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0)
                break;                  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) { /* It is a directory */
                ULOG_INFO("Dir: %s", fno.fname);
            } else { /* It is a file */
                ULOG_INFO("File: %s", fno.fname);
            }
        }
        f_closedir(&dir);
    }

    if (fatfs_is_file_exist("0:qfs.txt")) {
        ULOG_INFO("[fatfs] qfs.txt exist");
        fatfs_remove("0:qfs.txt");
    } else {
        ULOG_INFO("[fatfs] qfs.txt not exist");
    }

    if (fatfs_is_file_exist("0:test.txt")) {
        // Open file and read it contents
        FIL *pFile = &USERFile1;
        FRESULT res;
        UINT br;
        char rtext[100];

        res = f_open(pFile, "0:test.txt", FA_READ);
        if (res == FR_OK) {
            uint32_t crc = 0;
            if (CRC32_File(pFile, &crc) == 0) {
                ULOG_INFO("[fatfs] crc32 = %x", crc);
            } else {
                ULOG_INFO("[fatfs] crc32 failed");
            }

            uint32_t file_size = f_size(pFile);
            ULOG_INFO("[fatfs] file size = %u", file_size);

            //            for (uint32_t i = 0; i < file_size;) {
            //                memset(rtext, 0, sizeof(rtext));
            //                res = f_read(pFile, rtext, sizeof(rtext) - 1, &br);
            //                if (res == FR_OK) {
            //                    ULOG_INFO("[fatfs] read test.txt ok");
            //                    ULOG_INFO("[fatfs] test.txt content: %s %u", rtext, br);
            //                    i += br;
            //                } else {
            //                    ULOG_INFO("[fatfs] read test.txt failed");
            //                    break;
            //                }
            //            }

        } else {
            ULOG_INFO("[fatfs] open test.txt failed");
        }
        f_close(pFile);
    } else {
        ULOG_INFO("[fatfs] test.txt not exist");
    }

    res = f_opendir(&dir, path); /* Open the directory */

    if (res == FR_OK) {
        while (1) {
            res = f_readdir(&dir, &fno); /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0)
                break;                  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) { /* It is a directory */
                ULOG_INFO("Dir: %s", fno.fname);
            } else { /* It is a file */
                ULOG_INFO("File: %s", fno.fname);
            }
        }
        f_closedir(&dir);
    }

    // YmodemReceiveSessionInit(&ymodem_session, ymodem_read, ymodem_write);
}

void FatFs_DeInit()
{
    fatfs_unregister();
}

void fatfsTaskFun(void *argument)
{
    /* USER CODE BEGIN fatfsTaskFun */
    /* Infinite loop */
    FatFs_Init();

    for (;;) {

        // YmodemSessionPoll(&ymodem_session);
        // YmodemSessionResult_t result = YmodemSessionGetResult(&ymodem_session);
        // if (result != YMODEM_SESSION_RESULT_NONE)
        // {
        //   ULOG_INFO("[YMODEM] Result %d", (int)result);
        //   YmodemSessionClearResult(&ymodem_session);
        // }

        HAL_Delay(1);
    }

    FatFs_DeInit();
    /* USER CODE END fatfsTaskFun */
}

/**
 * @brief 计算文件的CRC32值。不会修改文件指针的位置。
 *
 * @param pFile 已经打开的文件对象指针。
 * @param crcRes
 * @return int 0 success, 1 error.
 */
int CRC32_File(FIL *pFile, uint32_t *crcRes)
{
    if (pFile == NULL || crcRes == NULL) {
        return 1; // 无效的参数
    }

    uint32_t crc = 0xFFFFFFFFUL; // 初始化CRC32值
    // 获取初始时的文件指针位置
    uint32_t filePos = f_tell(pFile);
    // 将文件指针移动到文件开头
    f_lseek(pFile, 0);

    // 动态分配缓冲区
    uint8_t *buffer = (uint8_t *)malloc(1024);
    if (buffer == NULL) {
        return 1; // 内存分配失败
    }

    // 读取文件并计算CRC32
    while (1) {
        UINT bytesRead = 0;
        if (f_read(pFile, buffer, 1024, &bytesRead) != FR_OK || bytesRead == 0) {
            break; // 读取文件失败或到达文件末尾
        }
        crc = CRC32_With(buffer, bytesRead, crc); // 使用现有的CRC32函数更新CRC32值
    }

    // 还原文件指针位置
    f_lseek(pFile, filePos);

    // 释放动态分配的缓冲区
    free(buffer);

    // 计算最终的CRC32值
    *crcRes = ~crc; // 取反以得到最终的CRC32值

    return 0; // 成功
}
/* USER CODE END Application */
