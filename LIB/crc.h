/**
 * @file crc.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-08
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#ifndef CRC_H
#define CRC_H
#include <stdint.h>
uint16_t CRC16_Modbus(const uint8_t *_pBuf, uint16_t _usLen);
uint32_t CRC32(const uint8_t *_pBuf, uint32_t _ulLen);
uint32_t CRC32_With(const uint8_t *_pBuf, uint32_t _ulLen, uint32_t _ulCRC);
#endif // !CRC_H
