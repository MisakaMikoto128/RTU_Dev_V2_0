/**
 * @file CHIP_W25Q512_test.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-09-23
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "CHIP_W25Q512_test.h"
#include "HDL_Flash_test.h"
#include "CHIP_W25Q512_QueueFileSystem.h"
#include "HDL_CPU_Time.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "test.h"
#include "scheduler.h"
// 测试集测不出数据覆盖问题
uint8_t var = 0;
uint8_t GET_TEST_DATA(uint32_t i)
{
    return ((uint8_t)((i) * (3 + i) + var));
}

static uint8_t test_buf[W25Q512_SECTOR_SIZE * 2];
static uint8_t read_buf[W25Q512_SECTOR_SIZE] = {0};

/**
 * @brief W25Q512读取/写入速度测试
 *
 */
void CHIP_W25Q512_io_rate()
{
    HDL_Flash_read(HDL_Flash_get_address_by_sector(40), &var, 1);
    var++;
    HDL_Flash_write(HDL_Flash_get_address_by_sector(40), &var, 1);

    Debug_Printf("\r\nvar : %d\r\n", var);

    Debug_Printf("\r\n%s\r\n", __func__);
    uint32_t total_test_size = W25Q512_SECTOR_SIZE * 10;
    uint32_t begin_tick = 0, end_tick = 0, exe_tick = 0;

    // 1.读取速度测试
    begin_tick = HAL_GetTick();
    for (size_t i = 0; i < total_test_size / W25Q512_SECTOR_SIZE; i++) {
        CHIP_W25Q512_read(0, test_buf, W25Q512_SECTOR_SIZE);
    }
    end_tick = HAL_GetTick();
    exe_tick = end_tick - begin_tick;
    Debug_Printf("[W25Q512] read time cost %.2f ms, totlal read %d KB\r\n", (exe_tick) / 1.0f, total_test_size / (1024));
    Debug_Printf("[W25Q512] read rate = %.2f MB/s\r\n", (total_test_size * 1.0 / (exe_tick) * 1000) / (1024 * 1024));

    // 2.写入速度测试
    // total_test_size = W25Q512_SECTOR_SIZE * 200;
    // begin_tick = HAL_GetTick();
    // for (size_t i = 0; i < total_test_size / W25Q512_SECTOR_SIZE; i++)
    // {
    //     CHIP_W25Q512_write(i * W25Q512_SECTOR_SIZE, test_buf, W25Q512_SECTOR_SIZE);
    // }
    // end_tick = HAL_GetTick();
    // exe_tick = end_tick - begin_tick;
    // Debug_Printf("[W25Q512] write time cost %.2f s, totlal read %d MB\r\n", (exe_tick) / 1000.0f, total_test_size / (1024 * 1024));
    // Debug_Printf("[W25Q512] write rate = %.2f KB/s\r\n", (total_test_size * 1.0 / (exe_tick)*1000) / (1024));
}

/**
 * @brief W25Q512写入和读取数据正确性测试。主要判断随机写入是否能够正确写入所有数据，且
 * 能够保证不会改变不在写入范围内的数据。
 *
 * @param start_address 起始写入地址。
 */
void CHIP_W25Q512_io_check(uint32_t start_address)
{
    Debug_Printf("\r\n%s\r\n", __func__);
    uint32_t total_test_size = W25Q512_SECTOR_SIZE * 2 - 12;
    uint32_t error_cnt       = 0;
#define PRE_TEST_SIZE 256
    start_address += PRE_TEST_SIZE;

    for (size_t i = 0; i < total_test_size; i++) {
        test_buf[i] = GET_TEST_DATA(i);
    }

    CHIP_W25Q512_read(start_address - PRE_TEST_SIZE, read_buf, PRE_TEST_SIZE);
    CHIP_W25Q512_read(start_address + total_test_size + PRE_TEST_SIZE, read_buf + PRE_TEST_SIZE, PRE_TEST_SIZE);

    CHIP_W25Q512_write(start_address, test_buf, total_test_size);
    for (size_t i = 0; i < total_test_size; i++) {
        test_buf[i] = 0;
    }

    CHIP_W25Q512_read(start_address, test_buf, total_test_size);
    for (size_t i = 0; i < total_test_size; i++) {
        if (test_buf[i] != GET_TEST_DATA(i)) {
            error_cnt++;
        }
    }
    Debug_Printf("[W25Q512] total test size: %u byte,error cnt: %u,passed %.2f%%\r\n",
                 total_test_size, error_cnt,
                 (total_test_size - error_cnt) / total_test_size * 100.0f);

    error_cnt = 0;
    CHIP_W25Q512_read(start_address - PRE_TEST_SIZE, test_buf, PRE_TEST_SIZE);
    CHIP_W25Q512_read(start_address + total_test_size + PRE_TEST_SIZE, test_buf + PRE_TEST_SIZE, PRE_TEST_SIZE);

    for (size_t i = 0; i < PRE_TEST_SIZE; i++) {
        if (test_buf[i] != read_buf[i]) {
            error_cnt++;
        }
    }
    Debug_Printf("[W25Q512] pretest error cnt: %u\r\n", error_cnt);

    error_cnt = 0;
    for (size_t i = PRE_TEST_SIZE; i < PRE_TEST_SIZE + PRE_TEST_SIZE; i++) {
        if (test_buf[i] != read_buf[i]) {
            error_cnt++;
        }
    }
    Debug_Printf("[W25Q512] last test error cnt: %u\r\n", error_cnt);
}

/**
 * @brief 测试多个扇区数据读写的正确性。
 *
 */
void CHIP_W25Q512_sector_io_check()
{
    Debug_Printf("\r\n%s\r\n", __func__);
    uint32_t error_cnt    = 0;
    uint32_t pass_sector  = 0;
    int total_test_sector = W25Q512_FLASH_SIZE / W25Q512_SECTOR_SIZE / 1024;
    for (int sector = 0; sector < total_test_sector; sector++) {
        for (int i = 0; i < W25Q512_SECTOR_SIZE; i++) {
            test_buf[i] = GET_TEST_DATA(i);
        }
        CHIP_W25Q512_write(sector * W25Q512_SECTOR_SIZE, test_buf, W25Q512_SECTOR_SIZE);
        CHIP_W25Q512_read(sector * W25Q512_SECTOR_SIZE, read_buf, W25Q512_SECTOR_SIZE);

        int i = 0;
        for (; i < W25Q512_SECTOR_SIZE; i++) {
            if (test_buf[i] != read_buf[i]) {
                error_cnt++;
                if (error_cnt < 3) {
                    Debug_Printf("[W25QXX] Sector %d ata check error i = %d\r\n", sector, i);
                    for (int j = 0; j < 8; j++) {
                        Debug_Printf("[W25QXX] r%X w%X \r\n", read_buf[j], test_buf[j]);
                    }
                }
                break;
            }
        }
        if (i == W25Q512_SECTOR_SIZE) {
            pass_sector++;
        }
    }
    Debug_Printf("[W25QXX] pass_sector = %.2f%%\r\n", pass_sector * 1.0 / total_test_sector * 100);
}

extern QFSHeader_t header;
void CHIP_W25Q512_QFS_test()
{
    HDL_CPU_Time_Init();
    ulog_init_user();
    CHIP_W25Q512_QFS_init();
    Debug_Printf("\r\n%s\r\n", __func__);
    uint32_t cpu_tick  = HDL_CPU_Time_GetTick();
    uint32_t var       = 1;
    uint8_t buf[20]    = {0};
    uint32_t write_len = 0;
    uint32_t error     = 0;
    int cnt            = 0;

    LoopFrequencyTest_t loop_frq_test = {
        .measure_time = 1000, // 测试1秒钟
        // 其他成员默认初始化为0.
    };
    while (1) {
        CHIP_W25Q512_QFS_handler();
        test_LoopFrequencyTest_handler(&loop_frq_test);

        if (test_LoopFrequencyTest_readable(&loop_frq_test)) {
            test_LoopFrequencyTest_show(&loop_frq_test, "Main");
            test_LoopFrequencyTest_reset(&loop_frq_test);
        }

        if (period_query(1, 10)) {
            sprintf((char *)buf, "var:%d", var++);
            write_len = CHIP_W25Q512_QFS_push(buf, strlen((char *)buf));
            if (write_len == 0) {
                if (CHIP_W25Q512_QFS_is_full()) {
                    Debug_Printf("[QFS Test]: QFS Full!\r\n");
                } else {
                    Debug_Printf("[QFS Test]: QFS Busy!\r\n");
                }
            }
        }

        if (period_query(0, 1000)) {
            uint32_t len = 0;
            // Warning: 如果写入速度远大于读取速度，那么很有可能始终无法读取出数据，解决方法就是
            // 让读取速度尽量接近读取速度，而且读取速度尽量保证打入写入速度，因为测试中发现，当写
            // 入和读取都在主循环中直接循环时读取数据的来源均是物理存储，无法从缓存队列中读取到数
            // 据。
            len = CHIP_W25Q512_QFS_asyn_read(test_buf, 30);
            if (len > 0) {
                test_buf[30] = 0;
                Debug_Printf("[QFS Test]: [D]:%s\r\n", test_buf);
                Debug_Printf("[QFS Test]: size after pop : %d\r\n", CHIP_W25Q512_QFS_size());
            }

            len = CHIP_W25Q512_QFS_asyn_read(test_buf, 30);
            if (len > 0) {
                test_buf[30] = 0;
                Debug_Printf("[QFS Test]: [D]:%s\r\n", test_buf);
                Debug_Printf("[QFS Test]: size after pop : %d\r\n", CHIP_W25Q512_QFS_size());
            }

            Debug_Printf("[QFS Test]: cache_queue_read_amount : %d byte\r\n", cache_queue_read_amount);
            Debug_Printf("[QFS Test]: physical_storage_read_amount : %d byte\r\n", physical_storage_read_amount);
        }

        // // 1s 执行一次
        // if ((HDL_CPU_Time_GetTick() - cpu_tick) > 100)
        // {
        //     if (CHIP_W25Q512_QFS_is_idle())
        //     {
        //         cpu_tick = HDL_CPU_Time_GetTick();
        //         QFSHeader_t header_tmp = {0};
        //         //更新当前的状态到内存中
        //         CHIP_W25Q512_read(0, (uint8_t *)&header_tmp, sizeof(QFSHeader_t));
        //         Debug_Printf("[QFS Test]: QFS save head font:%u,rear:%u\r\n", header_tmp.font_sec_numb, header_tmp.rear_sec_numb);
        //     }
        // }
    }
}
