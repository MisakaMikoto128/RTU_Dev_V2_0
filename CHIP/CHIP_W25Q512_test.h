/**
 * @file CHIP_W25Q512_test.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-05
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#ifndef CHIP_W25Q512_TEST_H
#define CHIP_W25Q512_TEST_H
#include "log.h"
#include "HDL_Uart.h"
#include "CHIP_W25Q512.h"
void CHIP_W25Q512_io_check(uint32_t start_address);
void CHIP_W25Q512_io_rate();
void CHIP_W25Q512_sector_io_check();
void CHIP_W25Q512_QFS_test();
#endif // !CHIP_W25Q512_TEST_H