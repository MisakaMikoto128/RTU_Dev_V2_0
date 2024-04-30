/**
 * @file mtime.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-09-14
 * @last modified 2023-09-14
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef MTIME_H
#define MTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* 自定义的时间结构体 */
typedef struct
{
    uint16_t nYear;
    uint8_t nMonth;
    uint8_t nDay;
    uint8_t nHour;
    uint8_t nMin;
    uint8_t nSec;
    uint8_t nWeek; /* 0 = Sunday */
    uint16_t wSub; // 亚秒
} mtime_t;

/**
 * @brief 根据给定的日期得到对应的星期
 *
 * @param year
 * @param month
 * @param day
 * @return uint8_t 0 = Sunday
 */
uint8_t mtime_get_week(uint16_t year, uint8_t month, uint8_t day);
void mtime_utc_sec_2_time(unsigned int utc_sec, mtime_t *result); // 根据UTC时间戳得到对应的日期
unsigned int mtime_2_utc_sec(mtime_t *currTime);                  // 根据时间计算出UTC时间戳
char *mtime_format(unsigned int utc_sec, char *pBuf);             // 根据UTC时间戳得到对应的日期字符串

void mtime_add_hours(mtime_t *currTime, unsigned int hours);
void mtime_sub_hours(mtime_t *currTime, unsigned int hours);
uint8_t mtime_is_equal(mtime_t *currTime, mtime_t *targetTime);

#ifdef __cplusplus
}
#endif
#endif //! MTIME_H