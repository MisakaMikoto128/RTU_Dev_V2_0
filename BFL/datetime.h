/**
 * @file datetime.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2024-04-11
 * @last modified 2024-04-11
 *
 * @copyright Copyright (c) 2024 Liu Yuanlin Personal.
 *
 */
#ifndef DATETIME_H
#define DATETIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "mtime.h"

void datetime_init(void);
void datetime_set_timezone(int8_t timezone);
int8_t datetime_get_timezone();
void datetime_set_localtime(mtime_t *mtime);
void datetime_get_localtime(mtime_t *mtime);
void datetime_set_local_timestamp(uint64_t timestamp);
uint64_t datetime_get_local_timestamp(void);
void datetime_set_unix_timestamp(uint64_t timestamp);
uint64_t datetime_get_unix_timestamp(void);
uint64_t datetime_get_unix_ms_timestamp(void);
bool datetime_has_synced(void);
#ifdef __cplusplus
}
#endif
#endif //! DATETIME_H
