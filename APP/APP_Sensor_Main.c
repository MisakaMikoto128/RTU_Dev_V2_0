/**
 * @file APP_Sensor17H.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-12-13
 * @description 0x17号传感器程序。
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#include "HDL_ADC.h"
#include "HDL_IWDG.h"
#include "HDL_Uart.h"
#include "HDL_RTC.h"
#include "CHIP_SHT30.h"
#include "BFL_RTU_Packet.h"
#include "BFL_LED.h"

#include "APP_Main.h"
#include "APP_Sampler.h"
#include "CHIP_W25Q512_QueueFileSystem.h"
#include "log.h"
#include "modbus_host.h"

#include "scheduler.h"
#include "mtime.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static void APP_Sensor_Main_handler();
static uint8_t BFL_4G_Module_writeable();
static void BFL_RTU_Packet_send(RTU_Packet_t *ppacket);
static void APP_Sampler_get_first_sample_var(RTU_Sampling_Var_t *pvar);
static uint8_t rtu_packet_buf[1024 * 2] = {0};
static uint8_t aBuf[1500];

void APP_Sensor_Main()
{
    RTU_Sampling_Var_t var;
    RTU_Sampling_Var_t var_header;
    RTU_Packet_t pkg          = {0};
    int pushed_sample_var_num = 0;
    CommunPara commPara;
    uint32_t dwLen;
    uint32_t led_period_recorder;
    HDL_CPU_Time_Init();
    HDL_RTC_Init();
    HDL_ADC_Init();
    HDL_ADC_Enable();
    HDL_WATCHDOG_Init(20);
    ulog_init_user();

    SetCommTestPara(&commPara); // 设置通信测试参数
    BFL_4G_Init(&commPara);     // 模块初始化
    BFL_RTU_Packet_init(&pkg, rtu_packet_buf, sizeof(rtu_packet_buf));
    BFL_LED_init();

    // 初始化RS485 3
    modbus_rtu_host_init(hModbusRTU3, 9600, 'N', 8);
    // 初始化RS485 4
    modbus_rtu_host_init(hModbusRTU4, 57600, 'N', 8);
    // 初始化RS485 5
    modbus_rtu_host_init(hModbusRTU5, 9600, 'N', 8);

    CHIP_W25Q512_QFS_init();
    CHIP_SHT30_Init();

    scheduler_init();
    APP_Sampler_init();
    // 设置姿态传感器请求数据的频率为50Hz
    APP_Sampler_set_sensor_freq(SENSOR_CODE_ATTITUDE_SENSOR, 50);
    // 设置当前传感器为姿态传感器
    APP_Sampler_change_current_enabled_sensor(SENSOR_CODE_ATTITUDE_SENSOR);

    APP_Sampler_get_first_sample_var(&var_header);

    while (1) {
        APP_Sensor_Main_handler();

        dwLen = BFL_4G_Read(1, aBuf, 1000);
        if (dwLen > 0) {
            // Uart_Write(COM1, aBuf, dwLen);
        }

        // 500 cputick执行一次
        if (period_query_user(&led_period_recorder, 500)) {
            BFL_LED_toggle(LED1);
        }

        HDL_WATCHDOG_Feed();

        if (BFL_4G_Module_writeable()) {
            if (APP_Sampler_read(&var)) {
                // 存放指定个数的数据到RTU数据包中(存放数据到RTU数据包的数据域)
                switch (pushed_sample_var_num) {
                    case 0: {
                        BFL_RTU_Packet_push_Sampling_Var(&pkg, &var_header);
                        BFL_RTU_Packet_push_Sampling_Var(&pkg, &var);
                        pushed_sample_var_num += 2;
                    } break;
                    case 50 - 1: // 存放了50给采样值了,最大60-1
                    {
                        BFL_RTU_Packet_push_Sampling_Var(&pkg, &var);
                        BFL_RTU_Packet_send(&pkg);
                        pushed_sample_var_num = 0;
                    } break;
                    default: {
                        // 这里最好自己预先确定有多少数据需要存放到数据域
                        BFL_RTU_Packet_push_Sampling_Var(&pkg, &var);
                        pushed_sample_var_num++;
                    } break;
                }
            }
        }
    }
}

/**
 * @brief 处理所有的轮询任务。
 *
 */
static void APP_Sensor_Main_handler()
{
    CHIP_W25Q512_QFS_handler();
    CHIP_SHT30_handler();
    scheduler_handler();
    modbus_rtu_handler();
    APP_Sampler_handler();
    BFL_4G_Roll();
}

/**
 * @brief 4G模块是否可以写（发送）数据。
 *
 * @return 1可写，0不可以写，
 */
static uint8_t BFL_4G_Module_writeable()
{
    uint32_t sendbuflen = BFL_4G_SendBufLen(1); // 查询待发送区长度 - sockid取 1或者2
    return sendbuflen < 1024 * 3;
}

/**
 * @brief 编码并发送数据包。
 *
 * @param ppacket
 */
static void BFL_RTU_Packet_send(RTU_Packet_t *ppacket)
{
    // 编码位数据包
    BFL_RTU_Packet_encoder(ppacket);
    // 发送字节流
    BFL_RTU_Packet_send_by_4G(ppacket, 1);
    // 每次使用完成后清空，防止上一次的数据带来的影响
    BFL_RTU_Packet_clear_buffer(ppacket);
}

static void APP_Sampler_get_first_sample_var(RTU_Sampling_Var_t *pvar)
{
    // 存放RTU设备代号
    // 清空var
    memset(pvar, 0, sizeof(RTU_Sampling_Var_t));
    // 放数据到采样点
    uint64_t rtu_devid = RTU_DEV_ID;
    // 放数据到采样点
    memcpy((void *)pvar->data, (void *)&rtu_devid, sizeof(uint64_t));
    // 明确采样点类型
    pvar->type = SENSOR_CODE_DEVICE_ID;
    // 编码采样点的时间和校验和
    RTU_Sampling_Var_encoder(pvar);
}