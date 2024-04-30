/**
 * @file BFL_RTU_Packet_test.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-08
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "BFL_RTU_Packet_test.h"

#include "scheduler.h"
#include "APP_RTU_Sampler.h"
#include <string.h>
#include <stdlib.h>
#include "APP_RTU_Sampler.h"
#include "BFL_RTU_Packet.h"
#include "APP_Main.h"

static uint8_t rtu_packet_buf[1024] = {0};
void BFL_RTU_Packet_test()
{
    HDL_CPU_Time_Init();
    ulog_init_user();

    RTU_Packet_t pkg = {0};
    BFL_RTU_Packet_init(&pkg, rtu_packet_buf, sizeof(rtu_packet_buf));
    RTU_Sampling_Var_t var;

    while (1) {
        if (period_query(0, 1000)) {
            // 存放数据到RTU数据包中(存放数据到RTU数据包的数据域)
            {

                {
                    // 存放RTU设备代号
                    // 清空var
                    memset((void *)&var, 0, sizeof(RTU_Sampling_Var_t));
                    // 放数据到采样点
                    uint64_t rtu_devid = RTU_DEV_ID;
                    // 放数据到采样点
                    memcpy((void *)var.data, (void *)&rtu_devid, sizeof(uint64_t));
                    // 明确采样点类型
                    var.type = SENSOR_CODE_DEVICE_ID;
                    // 编码采样点的时间和校验和
                    RTU_Sampling_Var_encoder(&var);
                    BFL_RTU_Packet_push_data(&pkg, (uint8_t *)&var, sizeof(var));
                }

                {
                    // 存放RTU设备代号
                    // 清空var
                    memset((void *)&var, 0, sizeof(RTU_Sampling_Var_t));
                    // 放数据到采样点
                    uint64_t rtu_devid = RTU_DEV_ID;
                    // 放数据到采样点
                    memcpy((void *)var.data, (void *)&rtu_devid, sizeof(uint64_t));
                    // 明确采样点类型
                    var.type = SENSOR_CODE_SOLAR_RADIATION;
                    // 编码采样点的时间和校验和
                    RTU_Sampling_Var_encoder(&var);
                    BFL_RTU_Packet_push_data(&pkg, (uint8_t *)&var, sizeof(var));
                }
            }
            // 编码位字节流
            BFL_RTU_Packet_encoder(&pkg);
            // 发送字节流
            sc_byte_buffer *buf = NULL;
            buf                 = BFL_RTU_Packet_get_buffer(&pkg);
            for (size_t i = 0; i < sc_byte_buffer_size(buf); i++) {
                if (i % 16 == 0) {
                    Debug_Printf("\r\n");
                }
                Debug_Printf("%02X ", sc_byte_buffer_at(buf, i));
            }
            Debug_Printf("\r\n");
            // 每次使用完成后清空，防止上一次的数据带来的影响
            BFL_RTU_Packet_clear_buffer(&pkg);
        }
    }
}