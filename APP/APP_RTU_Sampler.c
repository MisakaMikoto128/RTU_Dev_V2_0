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
#include "HDL_G4_RTC.h"
#include "log.h"
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @brief 计算RTU采样点数据的校验和。
 * @warning 这个函数只能在小端序下正常运行。
 * @param pvar
 * @return uint8_t 校验和。
 */
static uint8_t RTU_Sampling_Var_checksum(RTU_Sampling_Var_t *pvar)
{
	uint8_t checksum = 0;
	uint32_t sum = 0;
	uint8_t *pinput_data = (uint8_t *)pvar;
	for (int i = 0; i < sizeof(RTU_Sampling_Var_t) - 1; i++)
	{
		sum += pinput_data[i];
	}
	checksum = sum & 0xFF;
	return checksum;
}

/**
 * @brief 将字节流解析为一个采样点数据包。
 *
 * @param data 16字节的数据指针。
 * @param pvar 存放解析结果的数据包。
 * @return uint8_t 0解码成功，>0校验和错误，但是结果任然对应输入的数据。
 */
uint8_t RTU_Sampling_Var_decoder(const uint8_t *data, RTU_Sampling_Var_t *pvar)
{
	*pvar = *((RTU_Sampling_Var_t *)data);
	return RTU_Sampling_Var_checksum(pvar) == pvar->checksum;
}

/**
 * @brief 从采样点中获取本地日期时间对象。
 *
 * @param pvar
 * @param pdatetime
 */
void RTU_Sampling_Var_get_calibration_time_local(RTU_Sampling_Var_t *pvar, mtime_t *pdatetime)
{
	uint64_t datetime_ms = *((uint64_t *)(pvar->data));
	uint32_t datetime_sec = datetime_ms / 1000;
	uint32_t ms = datetime_ms - datetime_sec * 1000;
	mtime_utc_sec_2_time(datetime_sec + 8UL * 60UL * 60UL, pdatetime);
	pdatetime->wSub = HDL_G4_RTC_mSec2Subsec(ms);
}

/**
 * @brief 编码RTU采样点，完成时间和校验和编码。使用之前需要用户自己添加
 * 采样点的传感器编码，数据两个成员。
 * @warning 这个函数只能在小端序下正常运行。
 * @param pvar
 * @return uint8_t
 */
void RTU_Sampling_Var_encoder(RTU_Sampling_Var_t *pvar, uint8_t type, void *data, uint16_t len)
{
	len = len > sizeof(pvar->data) ? sizeof(pvar->data) : len;
	// 清空var
	memset(pvar, 0, sizeof(RTU_Sampling_Var_t));
	// 放数据到采样点对象
	memcpy(pvar->data, data, len);
	// 明确采样点类型
	pvar->type = type;
	// 编码采样点的时间和校验和
	pvar->datetime = HDL_G4_RTC_GetTimeTick(&pvar->ms);
	pvar->ms = HDL_G4_RTC_Subsec2mSec(pvar->ms);

	pvar->checksum = RTU_Sampling_Var_checksum(pvar);
}

/**
 * @brief 同RTU_Sampling_Var_encoder，但是自动获取编码时间。
 * 
 * @param pvar 
 * @param type 
 * @param data 
 * @param len 
 */
void RTU_Sampling_Var_encoder_no_timestamp(RTU_Sampling_Var_t *pvar, uint8_t type, void *data, uint16_t len)
{
	len = len > sizeof(pvar->data) ? sizeof(pvar->data) : len;
	// 清空var
	memset(pvar, 0, sizeof(RTU_Sampling_Var_t));
	// 放数据到采样点对象
	memcpy(pvar->data, data, len);
	// 明确采样点类型
	pvar->type = type;
	pvar->checksum = RTU_Sampling_Var_checksum(pvar);
}