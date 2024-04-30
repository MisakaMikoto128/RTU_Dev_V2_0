/**
 * @file Ymodem_if.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-05-27
 * @last modified 2023-05-27
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#include "Ymodem_if_prototype.h"
#include <stdio.h>
#include "HDL_CPU_Time.h"

bool YmodemIsFileExist(const char *file_name)
{
    FRESULT res;
    FILINFO fno;
    char path[256];
    snprintf(path, sizeof(path), "0:%s", file_name);
    res = f_stat(path, &fno);
    if (res == FR_OK) {
        return true;
    } else {
        return false;
    }
}
/**
 * @brief 删除文件。
 *
 * @param file_name
 * @return true 删除成功。
 * @return false 删除失败。
 */
bool YmodemFileRemove(const char *file_name)
{
    FRESULT res;
    char path[256];
    snprintf(path, sizeof(path), "0:%s", file_name);
    res = f_unlink(path);
    if (res == FR_OK) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief 打开文件，如果文件不存在则创建，如果文件存在则会以追加的方式打开。且可以读取。
 *
 * @param file_name
 * @return true 创建成功。
 * @return false 创建失败。
 */
bool YmodemFileOpen(YmodemFile_t *pFILE, const char *file_name)
{
    FIL *pFile = &pFILE->file;
    FRESULT res;
    char path[256];
    snprintf(path, sizeof(path), "0:%s", file_name);

    res = f_open(pFile, path, FA_OPEN_APPEND | FA_WRITE | FA_READ);
    if (res == FR_OK) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief
 *
 * @param file_name
 * @param data
 * @param size
 * @return uint32_t 写入文件的字节数。
 */
uint32_t YmodemFileAppend(YmodemFile_t *pFILE, uint8_t *data, uint32_t size)
{
    FIL *pFile = &pFILE->file;
    UINT bw;
    FRESULT res;
    res = f_write(pFile, data, size, &bw);
    if (res == FR_OK) {
        return bw;
    } else {
        return 0;
    }
}

/**
 * @brief 关闭文件。这个方法要保证传入NULL能够识别返回成功。
 *
 * @param pFILE
 * @return true 成功。
 * @return false 失败。
 */
bool YmodemFileClose(YmodemFile_t *pFILE)
{
    FIL *pFile = &pFILE->file;
    FRESULT res;
    res = f_close(pFile);
    if (res == FR_OK) {
        return true;
    } else {
        return false;
    }
}

uint32_t YmodemFileGetSize(YmodemFile_t *pFILE)
{
    FIL *pFile = &pFILE->file;
    FSIZE_t res;
    res = f_size(pFile);
    return res;
}