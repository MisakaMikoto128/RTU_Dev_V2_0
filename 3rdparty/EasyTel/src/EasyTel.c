/* Includes */
#include "EasyTel.h"
#include "SimpleDPP_port.h"
#include <string.h>
#include "crc.h"

static bool EsayTel_obj_write(EasyTelPoint *etp, const struct EasyTelMessage *msg);
static void EasyTelRevInnerCallback(EasyTelPoint *etp, struct EasyTelMessage *msg);
static void EasyTelAckInnerCallback(EasyTelPoint *etp, struct EasyTelMessage *msg);
uint32_t EsayTel_obj_write_prepare(EasyTelPoint *etp, const struct EasyTelMessage *msg);

#define SIMPLE_DPP_REV_BUFFER_SIZE 512
#define SIMPLE_DPP_SEND_BUFFER_SIZE 512
static __implemented sdp_byte __send_data[SIMPLE_DPP_SEND_BUFFER_SIZE];
static __implemented sdp_byte __recv_data[SIMPLE_DPP_REV_BUFFER_SIZE];

static uint16_t EasyTel_obj_get_msg_crc(struct EasyTelMessage *msg)
{
    msg->crc = CRC16_Modbus_start();
    msg->crc = CRC16_Modbus_update(msg->crc, (const uint8_t *)&msg->src, sizeof(msg->src));
    msg->crc = CRC16_Modbus_update(msg->crc, (const uint8_t *)&msg->dst, sizeof(msg->dst));
    msg->crc = CRC16_Modbus_update(msg->crc, (const uint8_t *)&msg->seq, sizeof(msg->seq));
    msg->crc = CRC16_Modbus_update(msg->crc, (const uint8_t *)&msg->type, sizeof(msg->type));
    msg->crc = CRC16_Modbus_update(msg->crc, (const uint8_t *)&msg->len, sizeof(msg->len));
    msg->crc = CRC16_Modbus_update(msg->crc, (const uint8_t *)msg->data, msg->len);
    return msg->crc;
}

static __implemented void SimpleDPPRecvCallback(void *obj, const sdp_byte *data, int len)
{

    SimpleDPP *sdp = (SimpleDPP *)obj;
    EasyTelPoint *etp = (EasyTelPoint *)sdp->obj;

    // 1. CRC 校验
    uint16_t crc = CRC16_Modbus(data, len - 2);
    uint16_t crc_recv = data[len - 2] | (data[len - 1] << 8);
    if (crc != crc_recv)
    {
        return;
    }

    // 2. 解析数据
    uint16_t src = data[0] | (data[1] << 8);
    uint16_t dst = data[2] | (data[3] << 8);
    uint16_t seq = data[4] | (data[5] << 8);
    uint8_t type = data[6];
    uint16_t data_len = data[7] | (data[8] << 8);
    uint8_t *data_ptr = (uint8_t *)&data[9];

    if (data_len > etp->curr_rev_msg.capacity)
    {
        return;
    }

    etp->curr_rev_msg.src = src;
    etp->curr_rev_msg.dst = dst;
    etp->curr_rev_msg.seq = seq;
    etp->curr_rev_msg.type = type;
    etp->curr_rev_msg.len = data_len;
    memcpy(etp->curr_rev_msg.data, data_ptr, data_len);
    etp->curr_rev_msg.crc = crc_recv;
    etp->curr_rev_msg.data[data_len] = '\0';

    // 3. 地址过滤
    if (dst == 0xFFFF)
    {
        // 3.1 广播地址，直接调用回调函数处理
        EasyTelRevInnerCallback(etp, &etp->curr_rev_msg);
        SDP_DEBUG(sdp, "[SDP] Recv broadcast message from %#x, len: %d, seq:%#x, data: %s", src, data_len, seq, etp->curr_rev_msg.data);
    }
    else if (dst == etp->addr)
    {
        // 3.2 本机地址，区分ACK和非ACK
        if (type == ETP_MSG_TYPE_DATA)
        {
            // 3.2.1 非ACK，首先回复SEQ+1的ACK，然后调用回调函数处理
            etp->curr_ack_msg.src = etp->addr;
            etp->curr_ack_msg.dst = src;
            etp->curr_ack_msg.seq = seq + 1;
            etp->curr_ack_msg.type = ETP_MSG_TYPE_ACK;
            // 交由用户填写数据
            EasyTelAckInnerCallback(etp, &etp->curr_rev_msg);
            etp->curr_ack_msg.crc = EasyTel_obj_get_msg_crc(&etp->curr_ack_msg);
            SDP_DEBUG(sdp, "[SDP] Recv message from %#x, len: %d, seq:%#x,data: %s", src, data_len, seq, etp->curr_rev_msg.data);
            EsayTel_obj_write(etp, &etp->curr_ack_msg);
            SDP_DEBUG(sdp, "[SDP] Send ACK to %#x seq:%#x", src, etp->curr_ack_msg.seq);
            EasyTelRevInnerCallback(etp, &etp->curr_rev_msg);
        }
        else if (type == ETP_MSG_TYPE_ACK)
        {
            // 3.2.2 ACK，处理发送任务
            if (etp->is_writing)
            {
                if (etp->curr_rev_msg.seq == (uint16_t)(etp->curr_send_msg.seq + 1))
                {
                    etp->write_result = ETP_WRITE_RESULT_OK;
                    etp->is_writing = false;

                    etp->curr_rev_ack_msg.src = etp->curr_rev_msg.src;
                    etp->curr_rev_ack_msg.dst = etp->curr_rev_msg.dst;
                    etp->curr_rev_ack_msg.seq = etp->curr_rev_msg.seq;
                    etp->curr_rev_ack_msg.type = ETP_MSG_TYPE_ACK;
                    etp->curr_rev_ack_msg.len = etp->curr_rev_msg.len;
                    memcpy(etp->curr_rev_ack_msg.data, etp->curr_rev_msg.data, etp->curr_rev_msg.len);
                    etp->curr_rev_ack_msg.data[etp->curr_rev_msg.len] = '\0';
                    etp->curr_rev_ack_msg.crc = etp->curr_rev_msg.crc;

                    SDP_DEBUG(sdp, "[SDP] Recv ACK from %#x seq:%#x", src, etp->curr_rev_ack_msg.seq);
                }
            }
        }
    }
    else
    {
        // 3.3 其他地址，不处理
        SDP_DEBUG(sdp, "[SDP] Recv other message from %#x, len: %d, seq:%#x, data: %s", src, data_len, seq, etp->curr_rev_msg.data);
    }
}

static __implemented void SimpleDPPRevErrorCallback(void *obj, SimpleDPPERROR error_code)
{
}

static void EasyTelRevInnerCallback(EasyTelPoint *etp, struct EasyTelMessage *msg)
{
    if (etp->rev_callback != NULL)
    {
        SDP_DEBUG(&etp->sdp, "[SDP] Call rev callback");
        etp->rev_callback(etp, msg);
    }
}

/**
 * @brief
 *
 * @param etp
 * @param msg 收到的请求消息。
 */
static void EasyTelAckInnerCallback(EasyTelPoint *etp, struct EasyTelMessage *msg)
{
    if (etp->ack_callback != NULL)
    {
        SDP_DEBUG(&etp->sdp, "[SDP] Call ACK callback");
        etp->ack_callback(etp, msg);
    }
    else
    {
        etp->curr_ack_msg.len = 0;
    }
}

EasyTelPoint *EasyTel_obj_create(const EasyTelPointAdapter_t *adapter, uint16_t addr)
{
    EasyTelPoint *etp = NULL;
    etp = (EasyTelPoint *)sdp_core_malloc(sizeof(EasyTelPoint));
    if (etp != NULL)
    {
        memset(etp, 0, sizeof(EasyTelPoint));
        SimpleDPPAdapter_t *sdp_adapter = (SimpleDPPAdapter_t *)sdp_core_malloc(sizeof(SimpleDPPAdapter_t));
        if (sdp_adapter == NULL)
        {
            EasyTel_obj_destroy(etp);
            etp = NULL;
            return etp;
        }
        memset(sdp_adapter, 0, sizeof(SimpleDPPAdapter_t));
        sdp_adapter->lock = adapter->lock;
        sdp_adapter->unlock = adapter->unlock;
        sdp_adapter->write = adapter->write;
        sdp_adapter->read = adapter->read;
        sdp_adapter->error = adapter->error;
        sdp_adapter->debug = adapter->debug;
        sdp_adapter->write_buf = __send_data;
        sdp_adapter->write_buf_capacity = SIMPLE_DPP_SEND_BUFFER_SIZE;
        sdp_adapter->read_buf = __recv_data;
        sdp_adapter->read_buf_capacity = SIMPLE_DPP_REV_BUFFER_SIZE;
        sdp_adapter->SimpleDPPRecvCallback = SimpleDPPRecvCallback;
        sdp_adapter->SimpleDPPRevErrorCallback = SimpleDPPRevErrorCallback;
        SimpleDPP_Constructor(&etp->sdp, sdp_adapter);

        etp->sdp.obj = etp;
        etp->seq = 0;
        etp->addr = addr;
        etp->write_result = ETP_WRITE_RESULT_NONE;
        etp->is_writing = false;
        etp->rev_callback = NULL;
        etp->have_write_task = false;
        etp->write_start_time = 0;
        etp->write_timeout = ETP_WRITE_TIMEOUT;
        etp->retry_times = 0;
        etp->retry_times_max = ETP_RETRY_TIMES_MAX;

        etp->curr_rev_msg.data = (uint8_t *)sdp_core_malloc(ETP_RECV_MSG_CAPACITY + 1);
        etp->curr_rev_msg.capacity = ETP_RECV_MSG_CAPACITY;
        etp->curr_send_msg.data = (uint8_t *)sdp_core_malloc(ETP_SEND_MSG_CAPACITY + 1);
        etp->curr_send_msg.capacity = ETP_SEND_MSG_CAPACITY;

        etp->curr_ack_msg.data = (uint8_t *)sdp_core_malloc(ETP_SEND_MSG_CAPACITY + 1);
        etp->curr_ack_msg.capacity = ETP_SEND_MSG_CAPACITY;
        etp->curr_rev_ack_msg.data = (uint8_t *)sdp_core_malloc(ETP_SEND_MSG_CAPACITY + 1);
        etp->curr_rev_ack_msg.capacity = ETP_SEND_MSG_CAPACITY;

        if (etp->curr_rev_msg.data == NULL || etp->curr_send_msg.data == NULL)
        {
            EasyTel_obj_destroy(etp);
            etp = NULL;
        }
    }
    return etp;
}

void EasyTel_obj_destroy(EasyTelPoint *etp)
{
    if (etp != NULL)
    {
        if (etp->curr_rev_msg.data != NULL)
        {
            sdp_core_free(etp->curr_rev_msg.data);
            etp->curr_rev_msg.data = NULL;
        }
        if (etp->curr_send_msg.data != NULL)
        {
            sdp_core_free(etp->curr_send_msg.data);
            etp->curr_send_msg.data = NULL;
        }
        if (etp->sdp.adapter != NULL)
        {
            sdp_core_free(etp->sdp.adapter);
            etp->sdp.adapter = NULL;
        }
        sdp_core_free(etp);
        etp = NULL;
    }
}

#define ETP_WRITE_RETRY(etp)                             \
    do                                                   \
    {                                                    \
        (etp)->have_write_task = true;                   \
        (etp)->write_start_time = SimpleDPP_getMsTick(); \
        (etp)->retry_times++;                            \
    } while (0)

static bool EsayTel_obj_is_timeout(EasyTelPoint *etp)
{
    uint32_t curr_time = SimpleDPP_getMsTick();
    return (curr_time - etp->write_start_time > etp->write_timeout);
}

static bool EsayTel_obj_write(EasyTelPoint *etp, const struct EasyTelMessage *msg)
{
    bool ret = false;
    int result = 0;
    send_datas_start(&etp->sdp);
    send_datas_add(&etp->sdp, (const sdp_byte *)&msg->src, sizeof(msg->src));
    send_datas_add(&etp->sdp, (const sdp_byte *)&msg->dst, sizeof(msg->dst));
    send_datas_add(&etp->sdp, (const sdp_byte *)&msg->seq, sizeof(msg->seq));
    send_datas_add(&etp->sdp, (const sdp_byte *)&msg->type, sizeof(msg->type));
    send_datas_add(&etp->sdp, (const sdp_byte *)&msg->len, sizeof(msg->len));
    send_datas_add(&etp->sdp, (const sdp_byte *)msg->data, msg->len);
    uint8_t crc[2];
    crc[0] = msg->crc & 0xFF;
    crc[1] = (msg->crc >> 8) & 0xFF;
    send_datas_add(&etp->sdp, (const sdp_byte *)crc, sizeof(crc));
    EsayTel_obj_write_prepare(etp, msg);
    result = send_datas_end(&etp->sdp);
    if (result != SIMPLEDPP_SENDFAILED)
    {
        ret = true;
    }
    return ret;
}

void EasyTel_obj_process(EasyTelPoint *etp)
{
    uint32_t curr_time = SimpleDPP_getMsTick();
    SimpeDPP_poll(&etp->sdp);

    if (etp->have_write_task)
    {
        etp->have_write_task = false;

        SDP_DEBUG(&etp->sdp, "[SDP] Write task");

        EsayTel_obj_write(etp, &etp->curr_send_msg);

        if (etp->curr_send_msg.dst == 0xFFFF)
        {
            etp->is_writing = false;
            etp->write_result = ETP_WRITE_RESULT_NONE;
        }
    }

    if (etp->is_writing)
    {
        if (EsayTel_obj_is_timeout(etp))
        {
            if (etp->retry_times < etp->retry_times_max)
            {
                ETP_WRITE_RETRY(etp);
                SDP_DEBUG(&etp->sdp, "[SDP] Retry times: %u", etp->retry_times);
            }
            else
            {
                // TODO: retry times over max,现在的策略就是一定非要发出来为止
                etp->is_writing = false;
                etp->write_result = ETP_WRITE_RESULT_TIMEOUT;
                SDP_DEBUG(&etp->sdp, "[SDP] Retry times over max");
            }
        }
    }
}

/**
 * @brief 判断现在是否可以写（请求）
 * 
 * @param etp 
 * @return true 
 * @return false 
 */
bool EasyTel_ctx_is_writeable(EasyTelPoint *etp)
{
    // is_writing用于判断是否正在发送数据，write_result用于判断发送响应的结果，由于广播消息没有响应，所以是否可写的条件为没有占用写资源且前面的响应结果已经被读取了。
    return etp->is_writing == false && etp->write_result == ETP_WRITE_RESULT_NONE;
}

bool EasyTel_obj_send(EasyTelPoint *etp, uint16_t dst, const void *data, uint32_t len)
{
    bool ret = false;
    if (EasyTel_ctx_is_writeable(etp) && (len < etp->curr_send_msg.capacity))
    {
        etp->curr_send_msg.src = etp->addr;
        etp->curr_send_msg.dst = dst;
        etp->curr_send_msg.seq = etp->seq++;
        etp->curr_send_msg.len = len;
        memcpy(etp->curr_send_msg.data, data, len);
        etp->curr_send_msg.data[len] = '\0';

        // CRC
        etp->curr_send_msg.crc = EasyTel_obj_get_msg_crc(&etp->curr_send_msg);

        etp->write_start_time = SimpleDPP_getMsTick();
        etp->have_write_task = true;
        etp->retry_times = 0;

        etp->is_writing = true;
        etp->write_result = ETP_WRITE_RESULT_NONE;
        ret = true;
    }
    return ret;
}

void EasyTel_obj_set_rev_callback(EasyTelPoint *etp, EasyTelRevCallback_t callback)
{
    etp->rev_callback = callback;
}

void EasyTel_obj_set_ack_callback(EasyTelPoint *etp, EasyTelRevCallback_t callback)
{
    etp->ack_callback = callback;
}

bool EasyTel_obj_have_response(EasyTelPoint *etp)
{
    return etp->write_result != ETP_WRITE_RESULT_NONE;
}

void EasyTel_obj_make_writeable(EasyTelPoint *etp)
{
    // etp->is_writing = false; ->内部已经处理为false
    etp->write_result = ETP_WRITE_RESULT_NONE;
}

uint32_t EasyTel_obj_ack_msg_push(EasyTelPoint *etp, const void *data, uint32_t len)
{
    uint32_t ret = 0;
    len = len > etp->curr_ack_msg.capacity ? etp->curr_ack_msg.capacity : len;
    memcpy(etp->curr_ack_msg.data, data, len);
    etp->curr_ack_msg.data[len] = '\0';
    etp->curr_ack_msg.len = len;
    ret = len;
    return ret;
}

__attribute__((weak)) uint32_t EsayTel_obj_write_prepare(EasyTelPoint *etp, const struct EasyTelMessage *msg)
{
    return 0;
}