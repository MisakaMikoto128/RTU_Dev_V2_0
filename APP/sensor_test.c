/**
 * @file APP_RTU_Sampler.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-10
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#include "APP_RTU_Sampler.h"
#include "HDL_RTC.h"
#include <math.h>
#include <test.h>
#define M_PI 3.14159265358979323846 // pi
#include "log.h"
#include "modbus_test.h"
#include "HDL_ADC.h"
#include "HDL_IWDG.h"
#include "CHIP_SHT30.h"
#include "CHIP_W25Q512_QueueFileSystem.h"
#include "scheduler.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @brief 初始化。
 *
 */
void sensor_testt()
{
    HDL_CPU_Time_Init();
    HDL_RTC_Init();
    ulog_init_user();
    // 初始化RS485 3
    modbus_rtu_host_init(hModbusRTU3, 9600, 'N', 8);
    // 初始化RS485 4
    modbus_rtu_host_init(hModbusRTU4, 57600, 'N', 8);
    // 初始化RS485 5
    modbus_rtu_host_init(hModbusRTU5, 9600, 'N', 8);

    CHIP_W25Q512_QFS_init();

    CHIP_SHT30_Init();
    HDL_ADC_Init();
    HDL_ADC_Enable(); // 使能

    while (1) {
        modbus_rtu_handler();
        if (period_query(0, 1000)) {
            if (modbus_rtu_host_read_cmd_03H(hModbusRTU3, 0x04, 0x0000, 1) == 0) {
                ULOG_DEBUG("Modbus 3 Send Failed! 0x04\r\n");
            }
        }

        {
            static uint16_t fi_white, fi_green, fi_red;

            if (modbus_rtu_host_match_ack_event(hModbusRTU3, 0x03, MODBUS_RTU_ANY_ADDRESS)) {
                switch (hModbusRTU3->slaveAddr) {
                    // 读取锚索计（#4）传感器
                    case 0x04: {
                        {
                            fi_red = hModbusRTU3->result.P[0];
                            modbus_rtu_host_clear_current_trans_event(hModbusRTU3);
                            if (modbus_rtu_host_read_cmd_03H(hModbusRTU3, 0x08, 0x0000, 1) == 0) {
                                ULOG_DEBUG("Modbus 3 Send Failed 0x08!\r\n");
                            }
                        }
                        break;
                    }

                    // 读取锚索计（#8）传感器
                    case 0x08: {

                        {
                            fi_green = hModbusRTU3->result.P[0];
                            modbus_rtu_host_clear_current_trans_event(hModbusRTU3);
                            if (modbus_rtu_host_read_cmd_03H(hModbusRTU3, 0x06, 0x0000, 1) == 0) {
                                ULOG_DEBUG("Modbus 3 Send Failed 0x06!\r\n");
                            }
                        }
                        break;
                    }

                    // 读取锚索计（#6）传感器
                    case 0x06: {
                        {
                            fi_white = hModbusRTU3->result.P[0];

                            double fi = (fi_white * 0.1f + fi_green * 0.1f + fi_red * 0.1f) / 3;
                            // double f0 = (2142.3f + 2198.8f + 2132.9f) / 3;
                            double f0     = 2158;
                            float P       = 2.177 * ((f0 * f0) - (fi * fi)) / (1000000.0);
                            int16_t u16_P = P * 100;

                            ULOG_DEBUG("[sensor test] P:%.3f fi_white : %.2f,"
                                       "fi_green : %.2f,fi_red : %.2f!",
                                       P, fi_white * 0.1f, fi_green * 0.1f, fi_red * 0.1f);
                        }
                        break;
                    }
                }
                modbus_rtu_host_clear_current_trans_event(hModbusRTU3);
            }
        }
    }
}
