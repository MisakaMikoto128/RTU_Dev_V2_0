/**
 * @file APP_Sampler.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-12-14
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#ifndef APP_SAMPLER_H
#define APP_SAMPLER_H
#include "sc_list.h"
#include <stdint.h>
#include "scheduler.h"
#include "mtime.h"
/**
 * @brief 传感器功能码定义
 *
┏━━━━━━━━━━━━━━┳━━━━━━━━━┓
┃ 传感器       	┃ 类型码  ┃
┡━━━━━━━━━━━━━━╇━━━━━━━━━┩
│ 风速、风向    │ 0x11    │
│ 太阳辐射   	│ 0x12    │
│ 锚索计   		│ 0x13    │
│ 电压电流  	│ 0x14    │x 4个 箱内、箱外
│ 位移          │ 0x15    │
│ 姿态          │ 0x16    │
│ 倾角     		│ 0x17    │
│ 拉压力     	│ 0x18    │
│ 温湿度       	│ 0x19    │x 2个
└──────────────┴─────────┘
 */
#define SENSOR_CODE_WIND_SPEED_AND_DIRECTION 0x11
#define SENSOR_CODE_SOLAR_RADIATION          0x12
#define SENSOR_CODE_ANCHOR_CABLE_GAUGE       0x13
#define SENSOR_CODE_VOLTAGE_CURRENT          0x14
#define SENSOR_CODE_DISPLACEMENT             0x15
#define SENSOR_CODE_ATTITUDE_SENSOR          0x16
#define SENSOR_CODE_INCLINATION_ANGLE_SENSOR 0x17
#define SENSOR_CODE_PULL_PRESSURE            0x18
#define SENSOR_CODE_TEMP_HUMI                0x19

#define SENSOR_CODE_DEVICE_ID                0x01

#define SENSOR_ERROR_VALUE                   0

#pragma pack(1)
typedef struct tagRTU_Sampling_Var_t {
    uint8_t type;      // 传感器类型
    uint32_t datetime; // 日期时间UTC时间戳：精度秒，24小时制。
    uint16_t ms;       // 毫秒
    uint8_t data[8];
    uint8_t checksum;
} RTU_Sampling_Var_t;
#define RTU_SAMPLING_VAR_T_SIZE 16U
#pragma pack()

typedef struct tagAPP_Sensor_t {
    uint8_t code;          // 传感器类型码
    char *name;            // 传感器名字
    SchedulerTask_t task;  // 传感器任务
    void (*handler)(void); // 传感器轮询方法
    struct sc_list next;
} APP_Sensor_t;

uint8_t RTU_Sampling_Var_checksum(RTU_Sampling_Var_t *pvar);
void RTU_Sampling_Var_encoder(RTU_Sampling_Var_t *pvar);
uint8_t RTU_Sampling_Var_decoder(const uint8_t *data, RTU_Sampling_Var_t *pvar);
void RTU_Sampling_Var_get_calibration_time_local(RTU_Sampling_Var_t *pvar, mtime_t *pdatetime);

void APP_RTU_Sampler_init();
void APP_RTU_Sampler_handler();
uint8_t APP_RTU_Sampler_read(RTU_Sampling_Var_t *pvar);

void APP_Sampler_init();
void APP_Sampler_disable_all_sensor_task();
void APP_Sampler_change_current_enabled_sensor(uint8_t sensor_code);
void APP_Sampler_enable_sensor(uint8_t sensor_code);
void APP_Sampler_set_sensor_freq(uint8_t sensor_code, int freq);
void APP_Sampler_handler();
uint8_t APP_Sampler_read(RTU_Sampling_Var_t *pvar);
#endif //! APP_SAMPLER_H