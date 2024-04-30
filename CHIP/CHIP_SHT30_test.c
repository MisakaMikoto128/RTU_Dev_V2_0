#include "CHIP_SHT30_test.h"

#include "HDL_CPU_Time.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "log.h"

/**
 * @brief SHT30阻塞式读取方法测试。
 *
 */
void CHIP_SHT30_poll_test()
{
    HDL_CPU_Time_Init();
    ulog_init_user();
    CHIP_SHT30_Init();

    uint32_t cpu_tick                 = HDL_CPU_Time_GetTick();
    LoopFrequencyTest_t loop_frq_test = {
        .measure_time = 1000, // 测试1秒钟
        // 其他成员默认初始化为0.
    };

    sht30Var_t sht30Var;

    while (1) {
        // 1s 执行一次
        if ((HDL_CPU_Time_GetTick() - cpu_tick) > 1000) {
            cpu_tick = HDL_CPU_Time_GetTick();

            CHIP_SHT30_Read(&sht30Var);
            Debug_Printf("[SHT30 Test]:temp:%d,humi:%d\r\n", sht30Var.temp, sht30Var.humi);
        }
    }
}

/**
 * @brief SHT30异步式读取方法测试。
 *
 */
void CHIP_SHT30_async_test()
{
    HDL_CPU_Time_Init();
    ulog_init_user();
    CHIP_SHT30_Init();

    uint32_t cpu_tick                 = HDL_CPU_Time_GetTick();
    LoopFrequencyTest_t loop_frq_test = {
        .measure_time = 1000, // 测试1秒钟
        // 其他成员默认初始化为0.
    };

    sht30Var_t sht30Var;

    while (1) {

        CHIP_SHT30_handler();

        test_LoopFrequencyTest_handler(&loop_frq_test);

        if (test_LoopFrequencyTest_readable(&loop_frq_test)) {
            test_LoopFrequencyTest_show(&loop_frq_test, "Main");
            test_LoopFrequencyTest_reset(&loop_frq_test);
        }

        // 1s 执行一次
        if ((HDL_CPU_Time_GetTick() - cpu_tick) > 1000) {
            cpu_tick = HDL_CPU_Time_GetTick();
            if (CHIP_SHT30_request() == 0) {
                Debug_Printf("[SHT30 Test]:SHT30 request error!\r\n");
            }
        }

        if (CHIP_SHT30_query_result() != 0) {
            CHIP_SHT30_get_result(&sht30Var);
            Debug_Printf("[SHT30 Test]:temp:%d,humi:%d\r\n", sht30Var.temp, sht30Var.humi);
            CHIP_SHT30_clear_all_event();
        }
    }
}