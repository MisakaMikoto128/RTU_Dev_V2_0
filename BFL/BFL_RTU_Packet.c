/**
 * @file BFL_RTU_Packet.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-08
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "BFL_RTU_Packet.h"
#include "crc.h"


/**
 * @brief 初始化RTU数据包，确定存放数据的缓存。
 *
 * @param ppacket 指向RTU数据包对象的指针。
 * @param data 缓冲区指针。
 * @param capacity 缓冲区容量，容量至少为4header+2crc+1dara = 6bytes。
 * @return uint8_t 0失败，1成功。
 */
uint8_t BFL_RTU_Packet_init(RTU_Packet_t *ppacket, byte *data, int capacity)
{
    uint8_t res = 0;
    if (capacity >= (RTU_PACKET_HEADER_LEN + 2 + 1))
    {
        ppacket->header.id = RTU_PACKET_ID;
        ppacket->header.rem_len = 0;
        sc_byte_buffer_init(&ppacket->buf, data, capacity - RTU_PACKET_HEADER_LEN);
        sc_byte_buffer_init(&ppacket->data_buf, data + RTU_PACKET_HEADER_LEN, capacity);
        res = 1;
    }
    return res;
}

/**
 * @brief 将现在的RTU数据包编码为将要发送的字节流，存放在数据包的buf成员中。
 * 这个方法在用户数据域数据存放完成后使用。
 * @warning 这个方法只能在小端序设备上运行。
 * @param ppacket 指向RTU数据包对象的指针。
 * @return uint8_t 1编码成功，0编码失败，缓存区大小不足。
 */
uint8_t BFL_RTU_Packet_encoder(RTU_Packet_t *ppacket)
{
    uint8_t res = 0;

    // 获取剩余部分长度，此时数据域长度以知
    ppacket->header.rem_len = sc_byte_buffer_size(&ppacket->data_buf) + RTU_PACKET_CRC_LEN;
    // 取低16位
    uint16_t id = ppacket->header.id;
    // 取低16位
    uint16_t rem_len = ppacket->header.rem_len;

    // 此时字节化缓冲还是空的，首先将头部存入字节化缓冲
    sc_byte_buffer_push_data(&ppacket->buf, (uint8_t *)&id, sizeof(uint16_t));
    sc_byte_buffer_push_data(&ppacket->buf, (uint8_t *)&rem_len, sizeof(uint16_t));
    // 连接数据域到字节化缓冲，因为是同一块内存，所以直接设置字节化缓冲的大小即可
    sc_byte_buffer_set_size(&ppacket->buf, sc_byte_buffer_size(&ppacket->data_buf) + sc_byte_buffer_size(&ppacket->buf));
    uint16_t crc = CRC16_Modbus((uint8_t*)sc_byte_buffer_data_ptr(&ppacket->buf), sc_byte_buffer_size(&ppacket->buf));
    
    //crc端序
    uint8_t crc_buf[RTU_PACKET_CRC_LEN] = {0};
    crc_buf[0] = (crc & (0xFF << 8)) >> 8;
    crc_buf[1] = (crc & (0xFF << 0)) >> 0;
    
    sc_byte_buffer_push_data(&ppacket->buf, (uint8_t *)&crc_buf, RTU_PACKET_CRC_LEN);
    sc_byte_buffer_set_size(&ppacket->data_buf, sc_byte_buffer_size(&ppacket->data_buf) + RTU_PACKET_CRC_LEN);
    return res;
}

/**
 * @brief 解码RTU数据包数据流到RTU数据包对象。这个方法不会拷贝数据到新的内存，只是将数据指针指向传入
 * 的数据缓存。
 *
 * @param ppacket 存放解析结果RTU数据包对象的指针。
 * @param data 数据
 * @param len 数据长度
 * @return uint8_t 0成功，>0失败,结果不再可以使用。
 */
uint8_t BFL_RTU_Packet_decoder(RTU_Packet_t *ppacket, byte *data, int len)
{
    uint8_t ret = 1;

    if (len >= RTU_PACKET_MIN_LEN)
    {
        sc_byte_buffer_init(&ppacket->buf, data, len);
        sc_byte_buffer_set_size(&ppacket->buf, len);
        // 计算CRC，这里数据的长度已经包含了CRC，这个带入能够计算得到0
        uint16_t crc = CRC16_Modbus((uint8_t*)sc_byte_buffer_data_ptr(&ppacket->buf), sc_byte_buffer_size(&ppacket->buf));
        if (crc == 0)
        {
            ppacket->header.id = *((uint16_t *)(data));
            ppacket->header.rem_len = *((uint16_t *)(data + 2));
            sc_byte_buffer_init(&ppacket->data_buf, data + RTU_PACKET_HEADER_LEN, len - RTU_PACKET_HEADER_LEN);
            ret = 0;
        }
    }
    return ret;
}

/**
 * @brief 清空RTU数据包的两个缓冲。
 *
 * @param ppacket
 */
void BFL_RTU_Packet_clear_buffer(RTU_Packet_t *ppacket)
{
    sc_byte_buffer_clear(&ppacket->buf);
    sc_byte_buffer_clear(&ppacket->data_buf);
}

uint8_t BFL_RTU_Packet_decoder(RTU_Packet_t *ppacket, byte *data, int len);

/**
 * @brief 向RTU 数据包数据域中存入数据。
 *
 * @param ppacket
 * @param data 数据所在内存的指针。
 * @param len 数据长度。
 * @return uin32_t 实际存入的字节数，为0就表示没有存进去。
 */
uint32_t BFL_RTU_Packet_push_data(RTU_Packet_t *ppacket, const byte *data, int len)
{
    if (len + 2 + sc_byte_buffer_size(&ppacket->data_buf) > sc_byte_buffer_capacity(&ppacket->data_buf))
    {
        len = sc_byte_buffer_capacity(&ppacket->data_buf) - 2 - sc_byte_buffer_size(&ppacket->data_buf);
    }
    return sc_byte_buffer_push_data(&ppacket->data_buf, data, len);
}

/**
 * @brief 向RTU 数据包数据域中存入一个采样值。
 *
 * @param ppacket
 * @param pvar 指向一个采样值的指针。
 * @return uint32_t 0失败没有存进去，1成功。
 */
uint32_t BFL_RTU_Packet_push_Sampling_Var(RTU_Packet_t *ppacket, RTU_Sampling_Var_t *pvar)
{
    uint32_t ret = 0;
    ret = BFL_RTU_Packet_push_data(ppacket, (uint8_t *)pvar, sizeof(RTU_Sampling_Var_t));
    return ret == sizeof(RTU_Sampling_Var_t);
}

/**
 * @brief 从RTU数据包获取采样点。将数据域看成一个采样点的数组。
 *
 * @param ppacket
 * @param pvar 存放采样点结果的对象的指针。
 * @return uint32_t 0正确读取到了采样数据点。1越界，2采样点校验错误。
 */
uint32_t BFL_RTU_Packet_get_Sampling_Var_at(RTU_Packet_t *ppacket, RTU_Sampling_Var_t *pvar, uint32_t index)
{
    uint32_t ret = 1; // 默认越界错误
    uint32_t rtu_sampling_var_t_arr_size = 0;
    rtu_sampling_var_t_arr_size = BFL_RTU_Packet_get_Sampling_Var_num(ppacket);
    if (index < rtu_sampling_var_t_arr_size)
    {
        ret = 0; // 解除越界错误

        uint8_t decode_res = 0;
        const uint8_t *data_ptr = sc_byte_buffer_data_ptr(&ppacket->data_buf);
        decode_res = RTU_Sampling_Var_decoder(data_ptr+index*RTU_SAMPLING_VAR_T_SIZE, pvar);
        
        if (decode_res > 0)
        {
            ret = 2;
        }
    }

    return ret;
}

/**
 * @brief 获取RTU数据包中有多少个采样点。
 *
 * @param ppacket
 * @return uint32_t 采样点个数。
 */
uint32_t BFL_RTU_Packet_get_Sampling_Var_num(RTU_Packet_t *ppacket)
{
    uint32_t ret = 0;
    ret = (ppacket->header.rem_len - RTU_PACKET_CRC_LEN) / RTU_SAMPLING_VAR_T_SIZE;
    return ret;
}

/**
 * @brief 获取字节化的RTU数据包数据流所存放存的缓存的指针。
 *
 * @param ppacket
 * @return sc_byte_buffer*
 */
sc_byte_buffer *BFL_RTU_Packet_get_buffer(RTU_Packet_t *ppacket)
{
    return &ppacket->buf;
}

/**
 * @brief 通过4G模块将数据包发送出去。
 *
 * @param sockid 套接字。
 */
void BFL_RTU_Packet_send_by_4G(RTU_Packet_t *ppacket, int sockid)
{
    sc_byte_buffer *buf = NULL;
    buf = BFL_RTU_Packet_get_buffer(ppacket);

    //BFL_4G_Write(2, (uint8_t *)sc_byte_buffer_data_ptr(buf), sc_byte_buffer_size(buf));
}
