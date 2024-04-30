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
#include "CHIP_W25Q512_QueueFileSystem.h"
#include "scheduler.h"
#include "APP_Sampler.h"
#include "BFL_RTU_Packet.h"
#include "test.h"
#include "mytime.h"
#include "log.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "modbus_host.h"

static bool task_sampler_request();
static void sensor_response_handler();

/*Config Here*/
APP_Sensor_t inclination_angle_sensor =
    {
        .code = SENSOR_CODE_INCLINATION_ANGLE_SENSOR,
        .name = "inclination angle sensor",
        .task = {
            .exe_times = SCHEDULER_EXE_TIMES_INF,
            .fun = task_sampler_request,
            .period = 1000,
            // 延时1500 tick开始调度，防止有些器件刚上电无法读取
            .delay_before_first_exe = 1500,
        },
        .handler = sensor_response_handler,
};

#define _MODBUS_HANDLER hModbusRTU4
#define _MODBUS_SLAVER_ADDR 0x51
#define _MODBUS_SLAVER_REGADDR 0x02BC
#define _MODBUS_READ_REG_NUM 2
static APP_Sensor_t *sensor = &inclination_angle_sensor;

static bool task_sampler_request()
{
    // 读取姿态（#50）传感器
    if (modbus_rtu_host_read_cmd_03H(_MODBUS_HANDLER, _MODBUS_SLAVER_ADDR, _MODBUS_SLAVER_REGADDR, _MODBUS_READ_REG_NUM) == 0)
    {
        LOG(LOG_DEBUG, "Modbus 4 Send Failed! devaddr : %#x transStageState: %#x RxCount: %#x", _MODBUS_SLAVER_ADDR, _MODBUS_HANDLER->transStageState, _MODBUS_HANDLER->RxCount);
    }
		return true;
}

static void sensor_response_handler()
{
    if (modbus_rtu_host_match_ack_event(_MODBUS_HANDLER, 0x03, _MODBUS_SLAVER_ADDR))
    {
        RTU_Sampling_Var_t var;
        // 清空var
        memset((void *)&var, 0, sizeof(RTU_Sampling_Var_t));
        // 放数据到采样点
        float m_vot[2] = {0};
        uint16_t Reg[2] = {0};
        modbus_rtu_get_data_uint16(_MODBUS_HANDLER, &Reg[0], 2);
        m_vot[0] = Reg[0] * 5 / 4096.0f;
        m_vot[1] = Reg[1] * 5 / 4096.0f;
        float m_angle[2] = {0};
        m_angle[0] = (m_vot[0] - 2.5f) / (5.0f / 30);
        m_angle[1] = (m_vot[1] - 2.5f) / (5.0f / 30);
        int16_t m_data[2] = {0};
        m_data[0] = m_angle[0] * 100;
        m_data[1] = m_angle[1] * 100;
        memcpy((void *)var.data, (void *)&m_data, 4);
        // 明确采样点类型
        var.type = SENSOR_CODE_INCLINATION_ANGLE_SENSOR;
        // 编码采样点的时间和校验和
        RTU_Sampling_Var_encoder(&var);
        // 将采样点推送到QFS
        uint32_t len = CHIP_W25Q512_QFS_push((uint8_t *)&var, sizeof(RTU_Sampling_Var_t));
        if (len < sizeof(RTU_Sampling_Var_t))
        {
            LOG(LOG_ERROR, "QFS push data failed!\r\n");
        }

        {
            static LoopFrequencyTest_t loop_frq_test =
                {
                    .measure_time = 1000,
                };

            test_LoopFrequencyTest_handler(&loop_frq_test);

            if (test_LoopFrequencyTest_readable(&loop_frq_test))
            {
                test_LoopFrequencyTest_show(&loop_frq_test, sensor->name);
                test_LoopFrequencyTest_reset(&loop_frq_test);
            }
        }
        modbus_rtu_host_clear_current_trans_event(_MODBUS_HANDLER);
    }
}
