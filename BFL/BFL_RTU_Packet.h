/**
 * @file BFL_RTU_Packet.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-08
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#ifndef BFL_RTU_PACKET_H
#define BFL_RTU_PACKET_H
#include <stdint.h>
#include "sc_byte_buffer.h"
#include "APP_RTU_Sampler.h"

#define RTU_PACKET_ID 0xAA55U
#define RTU_PACKET_HEADER_LEN 4 //字节
#define RTU_PACKET_CRC_LEN 2 //字节
#define RTU_PACKET_MIN_LEN 6 //字节
#define RTU_PACKET_MAX_DATA_FILED_LEN (60U*16U)
typedef struct tagRTU_PacketHeader_t
{
    uint16_t id;      //标识符
    uint16_t rem_len; //剩余部分长度
} RTU_PacketHeader_t;

typedef struct tagRTU_Packet_t
{
    RTU_PacketHeader_t header; //首部
    uint16_t crc;                 //校验
    sc_byte_buffer buf;           //字节化缓冲，用于存放字节化的RTU数据包，也就是要实际发送的数据。
    sc_byte_buffer data_buf;      //数据域+CRC校验域缓冲,包含存放CRC的两个字节,不包含头部。
    /*
    为了方便管理，buf和data_buf是指向同一块内存的，只不过buf的内存首部比data_buf内存首部低header大小个字节，
    也就是4字节，这部分用于存放header编码的结果，这样发送数据时直接拿buf的数据就行，而存数据时则可以使用
    sc_byte_buffer的方法。
    */
} RTU_Packet_t;


uint8_t BFL_RTU_Packet_init(RTU_Packet_t *ppacket, byte *data, int capacity);
uint8_t BFL_RTU_Packet_encoder(RTU_Packet_t *ppacket);
void BFL_RTU_Packet_clear_buffer(RTU_Packet_t *ppacket);
uint8_t BFL_RTU_Packet_decoder(RTU_Packet_t *ppacket, byte *data, int len);


uint32_t BFL_RTU_Packet_push_data(RTU_Packet_t *ppacket, const byte *data, int len);
uint32_t BFL_RTU_Packet_push_Sampling_Var(RTU_Packet_t *ppacket, RTU_Sampling_Var_t *pvar);
uint32_t BFL_RTU_Packet_get_Sampling_Var_at(RTU_Packet_t *ppacket, RTU_Sampling_Var_t *pvar,uint32_t index);
uint32_t BFL_RTU_Packet_get_Sampling_Var_num(RTU_Packet_t *ppacket);

sc_byte_buffer* BFL_RTU_Packet_get_buffer(RTU_Packet_t *ppacket);
void BFL_RTU_Packet_send_by_4G(RTU_Packet_t *ppacket, int sockid);
#endif // !BFL_RTU_PACKET_H