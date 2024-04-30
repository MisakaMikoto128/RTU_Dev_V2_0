/**
 * @file Ymodem_if.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-05-26
 * @last modified 2023-05-26
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef YMODEM_IF_H
#define YMODEM_IF_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "ff.h"
typedef struct tagYmodemFile_t {
    FIL file;
} YmodemFile_t;

#include "HDL_CPU_Time.h"
#define YMODEM_GET_MS_TICK()  HDL_CPU_Time_GetTick()

#define YMODEM_FILE_OVERWRITE (0x01)
#ifdef __cplusplus
}
#endif
#endif //! YMODEM_IF_H
