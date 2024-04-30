/**
 * @file APP_RTU_Sampler_test.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-11
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "APP_RTU_Sampler_test.h"
#include "APP_RTU_Sampler.h"
#include "CHIP_W25Q512_QueueFileSystem.h"
#include "scheduler.h"
#include "log.h"

#include <string.h>
#include "mtime.h"
/**
 * @brief RTU传感器数据采样器测试。
 *
 */
void APP_RTU_Sampler_test()
{
    LoopFrequencyTest_t loop_frq_test = {
        .measure_time = 1000, // 测试1秒钟
        // 其他成员默认初始化为0.
    };
    APP_RTU_Sampler_init();
    RTU_Sampling_Var_t var;
    uint32_t total_write_amount_bak = 0;
    while (1) {
        APP_RTU_Sampler_handler();
        if (test_LoopFrequencyTest_readable(&loop_frq_test)) {
            test_LoopFrequencyTest_show(&loop_frq_test, "Main");
            test_LoopFrequencyTest_reset(&loop_frq_test);
        }
        if (period_query(0, 10)) {
            /*
            内部是存放到QFS队列中的，但是考虑到有时候4G模块发送有时候可能时快时慢,
            APP_RTU_Sampler要保证即使没有读取数据，存储数据到QFS任然能够始终成功：
            即能够满足队列的一个特性，有剩余空间就必能写。
            1. 需要满足的第一个条件是QFS平均写入速率大于物理存储的平均写入速率，之前测得的
            W25Q512的平均写入速率为50KB/s.
            2. 因为写入速率存在不均匀性质，需要保证QFS写入缓存队列的容量足够，以达到峰值写入速度
            的要求。至于峰值写入速度多少、QFS写入缓存队列的容量要多大，这些受制于程序中产生数据
            的速度，和程序有关，实际中最简单的方法就是试错法，如果内存足够，稍微把QFS写入缓存队
            列的容量设置大一些就行。
            */

            if (APP_RTU_Sampler_read(&var)) {
                Debug_Printf("[APP_RTU_Sampler Test]:  start\r\ntype:%#x datetime%u ms:%d\r\n", var.type, var.datetime, var.ms);
                Debug_Printf("[APP_RTU_Sampler Test]: ");
                for (size_t i = 0; i < 8; i++) {
                    Debug_Printf("%#x ", var.data[i]);
                }

                Debug_Printf("[APP_RTU_Sampler Test]: end\r\n");
                Debug_Printf("[QFS Test]: cache_queue_read_amount : %d byte\r\n", cache_queue_read_amount);
                Debug_Printf("[QFS Test]: physical_storage_read_amount : %d byte\r\n", physical_storage_read_amount);
            }
        }

        // if (period_query(1, 1000))
        // {
        //     Debug_Printf("[QFS Test]: total_read_amount : %d byte\r\n", total_read_amount);
        //     Debug_Printf("[QFS Test]: total_write_amount : %d byte\r\n", total_write_amount);
        //     Debug_Printf("[QFS Test]: w rate : %.3f B/s\r\n", (total_write_amount - total_write_amount_bak) * 1.0f);
        //     total_write_amount_bak = total_write_amount;
        // }
    }
}

/**
 * @brief RTU采样器采样点数据编码测试，以风速、风向编码为例。
 *
 */
void APP_RTU_Sampler_encoder_test()
{
    /*
        采样点数据包字节流序号	采样点数据包字节流字节含义	例子	数值解释	数值含义解释
        0	类型码	0x11	类型码为0x11	传感器为风速风向传感器。
        1	秒时间戳低0字节	0x78	秒时间戳为0x12345678	从1970-1-1-0:0:0至今的秒数。
        2	秒时间戳低1字节	0x56
        3	秒时间戳低2字节	0x34
        4	秒时间戳低3字节	0x12
        5	毫秒时间戳低0字节	0x34	秒时间戳为0x1234	秒数下分毫秒数。
        6	毫秒时间戳低1字节	0x12
        7	数据字节0	0xBC	"共4有效字节：
        风速（单位m/s，2字节，0D-700D对应0.0m/s~70.0m/s）。
        风向（单位：°，2字节，0D-3599D对应0.0°~359.9°）。
        小端序存储。
        例子中:
        风速读取值为0x02BC = 700D
        风向读取值为0x0E0F = 3599D"	"风速 = 700 * 0.1 = 70.0m/s
        风向 = 3599 * 0.1 = 359.9°"
        8	数据字节1	0x02
        9	数据字节2	0x0F
        10	数据字节3	0x0E
        11	数据字节4	0x00
        12	数据字节5	0x00
        13	数据字节6	0x00
        14	数据字节7	0x00
        15	一字节校验和	0x46	"前15个字节求和取低8位
        0x46 = （0x11+0x78+0x56+0x34+0x12+0x34+0x12+0x0BC+0x02+0x0F+0x0E+0x00+0x00+0x00+0x00）&0xFF=0x246&0xFF"
    */
    APP_RTU_Sampler_init();
    RTU_Sampling_Var_t var = {0};

    // 清空var
    memset((void *)&var, 0, sizeof(RTU_Sampling_Var_t));
    // 放数据到采样点
    uint8_t *pdata = (uint8_t *)&var.data;
    pdata[0]       = 0xBC;
    pdata[1]       = 0x02;
    pdata[2]       = 0x0F;
    pdata[3]       = 0x0E;
    // 明确采样点类型
    var.type = SENSOR_CODE_WIND_SPEED_AND_DIRECTION;
    // 编码采样点的时间和校验和
    RTU_Sampling_Var_encoder(&var);

    Debug_Printf("[APP_RTU_Sampler Test]:  start\r\ntype:%#x datetime%u ms:%d\r\n", var.type, var.datetime, var.ms);
    mtime_t datetime = {0};

    utc_sec_2_mytime(var.datetime, &datetime);
    Debug_Printf("%04d-%02d-%02d %02d:%02d:%02d %02d\r\n", datetime.nYear, datetime.nMonth, datetime.nDay, datetime.nHour, datetime.nMin, datetime.nSec, datetime.wSub);
    Debug_Printf("timestamp = %u\r\n", var.datetime);
    Debug_Printf("[APP_RTU_Sampler Test]: ");
    for (size_t i = 0; i < 8; i++) {
        Debug_Printf("%02X ", var.data[i]);
    }

    Debug_Printf("[APP_RTU_Sampler Test]:\r\n");
    pdata = (uint8_t *)&var;
    for (size_t i = 0; i < sizeof(RTU_Sampling_Var_t); i++) {
        Debug_Printf("%02X ", pdata[i]);
    }
    Debug_Printf("[APP_RTU_Sampler Test]: end\r\n");
    while (1) {
    }
}