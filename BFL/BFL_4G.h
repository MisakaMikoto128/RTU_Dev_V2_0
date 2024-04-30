/**
 * @file BFL_4G.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-09-17
 * @last modified 2023-09-17
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef BFL_4G_H
#define BFL_4G_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define SOCKET0 0
#define SOCKET1 1
#define SOCKET2 2

/**
 * @brief 4G模块初始化。
 *
 * @note 这是参数配置和与4G模块的通信的初始化，不实际操作4G模块。
 *
 * @param PDP_type 如果为NULL将使用默认的"IP"。
 * "IP" 互联网协议版本 4（IETF STD 5）
 * "PPP" 点对点协议（IETF STD 51）
 * "IPV6" 互联网协议版本 6
 * "IPV4V6" 引入虚拟 <PDP_type> 处理双 IP 堆栈 UE 功能
 * @param APN 如果为NULL将使用默认的APN,CMIOT。具体查询运营商提供的APN。
 */
void BFL_4G_Init(const char *PDP_type, const char *APN);

/**
 * @brief Socket-TCP参数初始化。
 *
 * @param sockid
 * @param hostAddr
 * @param port
 * @return int32_t
 */
int32_t BFL_4G_TCP_Init(int sockid, const char *hostAddr, uint16_t port);
uint32_t BFL_4G_TCP_Write(int sockid, uint8_t *writeBuf, uint32_t uLen);
uint32_t BFL_4G_TCP_Read(int sockid, unsigned char *pBuf, uint32_t uiLen);
uint32_t BFL_4G_TCP_Writeable(int sockid);
uint32_t BFL_4G_TCP_Readable(int sockid);

/**
 * @brief 模块轮询处理
 *
 */
void BFL_4G_Poll();

/**
 * @brief
 *
 * @param mqttPubId
 * @param payload 指向数据荷载的指针。
 * @param uiLen 数据荷载的长度。
 * @return uint32_t 写入是否成功。如果无法分配到内存或者写入缓存队列满了会失败。
 */

/**
 * @brief 向指定发布ID下写入数据。
 *
 * @param mqttPubId
 * @param payload 指向数据荷载的指针。
 * @param uiLen 数据荷载的长度。
 * @param qos
 * @param retain
 * @return uint32_t 写入是否成功。如果无法分配到内存或者写入缓存队列满了会失败。
 */
uint32_t BFL_4G_MQTT_Write(int mqttPubId, const uint8_t *payload, uint32_t uiLen, uint8_t qos, uint8_t retain);

/**
 * @brief 指定发布ID下是否能够写入数据。仅检查写入队列是否还有空位。
 *
 * @param mqttPubId
 * @return true
 * @return false
 */
bool BFL_4G_MQTT_Writeable(int mqttPubId);

/**
 * @brief 读取指定订阅ID下的消息。
 *
 * @param mqttSubId
 * @param pBuf
 * @param uiLen
 * @return uint32_t 读取是否成功，0代表失败，>0表示成功，为实际接收到的消息的长度。如果失败那么不会修改@pBuf中的内容。
 */
uint32_t BFL_4G_MQTT_Read(int mqttSubId, uint8_t *pBuf, uint32_t uiLen);

/**
 * @brief 指定订阅ID下是否有消息可以读取。
 *
 * @param mqttSubId
 * @return true
 * @return false
 */
bool BFL_4G_MQTT_Readable(int mqttSubId);

/**
 * @brief 设置校准时间的回调函数。
 *
 * @param setCalibrateTimeByUtcSecondsCb
 */
void BFL_4G_SetCalibrateTimeByUtcSecondsCb(void (*setCalibrateTimeByUtcSecondsCb)(uint64_t utcSeconds));

/**
 * @brief 启动一次时间校准过程。
 *
 */
void BFL_4G_StartCalibrateTimeOneTimes();

#ifdef __cplusplus
}
#endif
#endif //! BFL_4G_H
