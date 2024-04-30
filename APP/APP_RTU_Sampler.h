/**
 * @file APP_RTU_Sampler.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-10
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#ifndef APP_RTU_SAMPLER_H
#define APP_RTU_SAMPLER_H
#include <stdint.h>
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
#define SENSOR_CODE_SOLAR_RADIATION 0x12
#define SENSOR_CODE_ANCHOR_CABLE_GAUGE 0x13
#define SENSOR_CODE_VOLTAGE_CURRENT 0x14
#define SENSOR_CODE_DISPLACEMENT 0x15
#define SENSOR_CODE_ATTITUDE_SENSOR 0x16
#define SENSOR_CODE_INCLINATION_ANGLE_SENSOR 0x17
#define SENSOR_CODE_PULL_PRESSURE 0x18
#define SENSOR_CODE_TEMP_HUMI 0x19

//x -> 1-8 表示1-8号位移传感器
#define SENSOR_CODE_DISPLACEMENTx(x) (0x20 + x)

#define SENSOR_CODE_DEVICE_ID 0x01

#define SENSOR_ERROR_VALUE 0

#pragma pack(1)
typedef struct tagRTU_Sampling_Var_t
{
    uint8_t type;      // 传感器类型
    uint32_t datetime; // 日期时间UTC时间戳：精度秒，24小时制。
    uint16_t ms;       // 毫秒
    uint8_t data[8];
    uint8_t checksum;
} RTU_Sampling_Var_t;
#define RTU_SAMPLING_VAR_T_SIZE 16U
#pragma pack()

void RTU_Sampling_Var_encoder(RTU_Sampling_Var_t *pvar, uint8_t type, void *data, uint16_t len);
void RTU_Sampling_Var_encoder_no_timestamp(RTU_Sampling_Var_t *pvar, uint8_t type, void *data, uint16_t len);
uint8_t RTU_Sampling_Var_decoder(const uint8_t *data, RTU_Sampling_Var_t *pvar);
void RTU_Sampling_Var_get_calibration_time_local(RTU_Sampling_Var_t *pvar, mtime_t *pdatetime);
#endif // !APP_RTU_SAMPLER_H
