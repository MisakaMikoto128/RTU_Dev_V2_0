/**
 * @file ymodem_if_prototype.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-05-27
 * @last modified 2023-05-27
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef YMODEM_IF_PROTOTYPE_H
#define YMODEM_IF_PROTOTYPE_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ymodem_if.h"

bool YmodemIsFileExist(const char *file_name);
/**
 * @brief 删除文件。
 *
 * @param file_name
 * @return true 删除成功。
 * @return false 删除失败。
 */
bool YmodemFileRemove(const char *file_name);

/**
 * @brief 打开文件，如果文件不存在则创建，如果文件存在则会以追加的方式打开。
 *
 * @param file_name
 * @return true 创建成功。
 * @return false 创建失败。
 */
bool YmodemFileOpen(YmodemFile_t *pFILE, const char *file_name);

/**
 * @brief
 *
 * @param file_name
 * @param data
 * @param size
 * @return uint32_t 写入文件的字节数。
 */
uint32_t YmodemFileAppend(YmodemFile_t *pFILE, uint8_t *data, uint32_t size);

/**
 * @brief 关闭文件。这个方法要保证传入NULL能够识别返回成功。
 *
 * @param pFILE
 * @return true 成功。
 * @return false 失败。
 */
bool YmodemFileClose(YmodemFile_t *pFILE);

/**
 * @brief 获取文件大小。
 *
 * @param pFILE
 * @return uint32_t 文件大小。失败返回0。
 */
uint32_t YmodemFileGetSize(YmodemFile_t *pFILE);
#ifdef __cplusplus
}
#endif
#endif //! YMODEM_IF_PROTOTYPE_H
