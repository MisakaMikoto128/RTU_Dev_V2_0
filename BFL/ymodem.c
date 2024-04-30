/**
 * @file Ymodem.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-05-26
 * @last modified 2023-05-26
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#include "Ymodem.h"
#include "ymodem_if_prototype.h"
#include <string.h>
#include <stdio.h>

/* ASCII control codes: */
#define SOH (0x01) /* start of 128-byte data packet */
#define STX (0x02) /* start of 1024-byte data packet */
#define EOT (0x04) /* end of transmission */
#define ACK (0x06) /* receive OK */
#define NAK (0x15) /* receiver error; retry */
#define CAN (0x18) /* two of these in succession aborts transfer */
#define CNC (0x43) /* character 'C' */

/* pad character*/
#define CPMEOF                    (0x1A)

#define YMODEM_IS_CONTROL_CHAR(c) ((c) == SOH || (c) == STX || (c) == EOT || (c) == ACK || (c) == NAK || (c) == CAN || (c) == CNC)

/**
 * @brief 使用该方法需要保证Message是YMODEM_MESSAGE_TYPE_CMDS
 */
#define YMODEM_MESSAGE_TYPE_CMDS_IS(buf, CMD1, CMD2) (buf[0] == CMD1 && buf[1] == CMD2)
#define YMODEM_FRAME_SIZE_BY_HEADER(frame_header)    ((frame_header) == SOH ? (128 + (YMODEM_FRAME_HEADER_LEN + YMODEM_FRAME_CHECK_LEN)) : (1024 + (YMODEM_FRAME_HEADER_LEN + YMODEM_FRAME_CHECK_LEN)))

YmodemSessionResult_t YmodemPushMessageToReceiveSession(YmodemSession_t *session, uint8_t *data, uint32_t size);
/**
 * @brief 计算crc16校验码.
 *
 * @param buf
 * @param count
 * @return uint16_t
 */
static uint16_t Ymodem_CRC16(const uint8_t *buf, size_t count)
{
    uint16_t crc = 0;
    int i;

    while (count--) {
        crc = crc ^ *buf++ << 8;

        for (i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = crc << 1 ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

static YmodemMessageType_t YmodemMessageCheck(const uint8_t *data, uint32_t size)
{
    if (data == NULL || size == 0) {
        return YMODEM_MESSAGE_TYPE_UNKNOWN;
    }

    if (size == 1) {
        if (YMODEM_IS_CONTROL_CHAR(data[0])) {
            return YMODEM_MESSAGE_TYPE_CMD;
        } else {
            return YMODEM_MESSAGE_TYPE_UNKNOWN;
        }
    }
    // 按照《XMODEM/YMODEM PROTOCOL REFERENCE》只会有两个控制字符会连续发送，但是许多上位机主动结束发送时会发送连续4个CAN控制字符，有些还会附带一些不知道什么作用的字符。
    // 这里16是估算可能的没有按照规定写的发送程序
    else if (size > 1 && size < 16) {
        YmodemMessageType_t type = YMODEM_MESSAGE_TYPE_CMDS;
        // 两个CAN或者ACK,CNC
        if (size == 2) {
            for (uint32_t i = 0; i < size; i++) {
                if (!YMODEM_IS_CONTROL_CHAR(data[i])) {
                    type = YMODEM_MESSAGE_TYPE_UNKNOWN;
                    break;
                }
            }
        } else {
            // CAN CAN CAN CAN 或者 CAN CAN CAN CAN X X X X
            // 一般来说最多会有四个连续的CAN控制字符
            size = size > 4 ? 4 : size;
            for (uint32_t i = 0; i < size; i++) {
                if (!YMODEM_IS_CONTROL_CHAR(data[i])) {
                    type = YMODEM_MESSAGE_TYPE_UNKNOWN;
                    break;
                }
            }
        }
        return type;
    } else if (size == YMODEM_FRAME_SIZE_BY_HEADER(SOH)) {
        uint16_t crc = Ymodem_CRC16(data + YMODEM_FRAME_HEADER_LEN, YMODEM_FRAME_SIZE_BY_HEADER(SOH) - YMODEM_FRAME_HEADER_LEN);
        if (crc == 0) {
            if (data[1] == 0x00) {
                for (uint32_t i = 0; i < YMODEM_FRAME_SIZE_BY_HEADER(SOH); i++) {
                    if (data[i + YMODEM_FRAME_HEADER_LEN] != 0x00) {
                        return YMODEM_MESSAGE_TYPE_128_BLOCK;
                    }
                }
            }
            return YMODEM_MESSAGE_TYPE_128_NUL_BLOCK;
        } else {
            return YMODEM_MESSAGE_TYPE_UNKNOWN;
        }
    } else if (size == YMODEM_FRAME_SIZE_BY_HEADER(STX)) {
        uint16_t crc = Ymodem_CRC16(data + YMODEM_FRAME_HEADER_LEN, YMODEM_FRAME_SIZE_BY_HEADER(STX) - YMODEM_FRAME_HEADER_LEN);
        if (crc == 0) {
            return YMODEM_MESSAGE_TYPE_1K_BLOCK;
        } else {
            return YMODEM_MESSAGE_TYPE_UNKNOWN;
        }
    }
    return YMODEM_MESSAGE_TYPE_UNKNOWN;
}

void YmodemReceiveSessionInit(YmodemSession_t *session, size_t (*read)(uint8_t *buf, size_t size), size_t (*write)(const uint8_t *buf, size_t size))
{
    if (session == NULL) {
        return;
    }
    session->timeout_                   = 2000;
    session->last_send_message_tick_ms_ = 0;
    session->next_expected_sequence_    = 0;
    session->retry_count_               = 0;
    session->retry_max_count_           = 8;
    session->state_                     = YMODEM_SESSION_CLOSED;
    session->result_                    = YMODEM_SESSION_RESULT_NONE;
    memset(session->buffer_, 0, sizeof(session->buffer_));
    session->buffer_size_ = 0;
    session->read         = read;
    session->write        = write;
    memset(&session->file_name_, 0, sizeof(session->file_name_));
    session->file_size_   = 0;
    session->file_offset_ = NULL;

    session->received_cancle_count_ = 0;
    session->file_is_open_          = false;
    session->last_read_tick_ms_     = 0;
}

bool YmodemReceiveSessionOpen(YmodemSession_t *session, size_t (*read)(uint8_t *buf, size_t size), size_t (*write)(const uint8_t *buf, size_t size))
{
    if (session == NULL) {
        return false;
    }

    if (session->state_ != YMODEM_SESSION_CLOSED) {
        return false;
    }

    YmodemReceiveSessionInit(session, read, write);
    session->state_ = YMODEM_RECEIVE_WAIT_HEADER;
    return true;
}

void YmodemReceiveSessionCancle(YmodemSession_t *session)
{
    if (session == NULL) {
        return;
    }

    if (session->state_ == YMODEM_SESSION_CLOSED) {
        return;
    }

    const uint8_t cmds_buf[4] = {CAN, CAN, CAN, CAN};
    session->write(cmds_buf, sizeof(cmds_buf));
    session->state_  = YMODEM_SESSION_CLOSED;
    session->result_ = YMODEM_SESSION_RESULT_CANCLE;
}

YmodemSessionResult_t YmodemSessionGetResult(YmodemSession_t *session)
{
    if (session == NULL) {
        return YMODEM_SESSION_RESULT_NONE;
    }
    return session->result_;
}

void YmodemSessionClearResult(YmodemSession_t *session)
{
    if (session == NULL) {
        return;
    }
    session->result_ = YMODEM_SESSION_RESULT_NONE;
}

static void YmodemReceiveSessionFileHandleBegin(YmodemSession_t *session, uint8_t *data, uint32_t size)
{
    // eg . SHO 00 FF "f00.c" 1234 00 00 00 CRCH CRCL
    char *file_name = (char *)(data + YMODEM_FRAME_HEADER_LEN);

    strncpy(session->file_name_, file_name, YMODEM_FRAME_SIZE_BY_HEADER(SOH));
    session->file_name_[YMODEM_FRAME_SIZE_BY_HEADER(SOH)] = '\0';

    size_t file_name_len = strlen(session->file_name_);
    uint32_t file_size   = 0;
    sscanf((char *)(data + YMODEM_FRAME_HEADER_LEN + file_name_len + 1), "%d", &file_size);

#if YMODEM_FILE_OVERWRITE
    if (YmodemIsFileExist(session->file_name_)) {
        YmodemFileRemove(session->file_name_);
    }
#endif
    session->file_is_open_ = YmodemFileOpen(&session->file, session->file_name_);
    if (session->file_is_open_ == false) {
        YmodemReceiveSessionCancle(session);
        session->result_ = YMODEM_SESSION_RESULT_ERROR;
        session->state_  = YMODEM_SESSION_CLOSED;
    } else {
        session->file_size_   = file_size;
        session->file_offset_ = 0;
        session->result_      = YMODEM_SESSION_RESULT_NONE;
    }
}

static void YmodemPushDataToFile(YmodemSession_t *session, uint8_t *data, uint32_t size)
{
    uint8_t cmd            = data[0];
    size_t valid_data_size = 0;
    if (session->file_size_ - session->file_offset_ < YMODEM_FRAME_SIZE_BY_HEADER(cmd)) {
        valid_data_size = session->file_size_ - session->file_offset_;
    } else {
        valid_data_size = YMODEM_FRAME_SIZE_BY_HEADER(cmd);
    }

    YmodemFileAppend(&session->file, data + YMODEM_FRAME_HEADER_LEN, valid_data_size);
    session->file_offset_ += valid_data_size;
}

/**
 * @brief 接收会话处理函数。该方法在接收到消息时调用。
 *
 * @param session
 * @param data
 * @param size
 * @return YmodemSessionResult_t
 */
YmodemSessionResult_t YmodemPushMessageToReceiveSession(YmodemSession_t *session, uint8_t *data, uint32_t size)
{
    if (session == NULL || data == NULL || size == 0 || session->state_ == YMODEM_SESSION_CLOSED) {
        return YMODEM_SESSION_RESULT_NONE;
    }

    YmodemMessageType_t msg_type = YmodemMessageCheck(data, size);
    uint8_t sequence             = 0;
    uint8_t cmd                  = 0;
    cmd                          = data[0];
    if (msg_type == YMODEM_MESSAGE_TYPE_128_BLOCK || msg_type == YMODEM_MESSAGE_TYPE_1K_BLOCK) {
        sequence = data[1];
    }

    // 发送方主动取消传输处理
    if (msg_type == YMODEM_MESSAGE_TYPE_CMD && cmd == CAN) {
        session->received_cancle_count_++;
        if (session->received_cancle_count_ >= 2) {
            session->result_ = YMODEM_SESSION_RESULT_CANCLE;
            session->state_  = YMODEM_SESSION_CLOSED;
            return YMODEM_SESSION_RESULT_CANCLE;
        } else {
            session->result_ = YMODEM_SESSION_RESULT_NONE;
            return YMODEM_SESSION_RESULT_NONE;
        }
    }

    switch (session->state_) {
        case YMODEM_RECEIVE_WAIT_HEADER:
            if (msg_type == YMODEM_MESSAGE_TYPE_128_BLOCK && sequence == 0) {
                session->next_expected_sequence_ = sequence + 1;
                session->state_                  = YMODEM_RECEIVE_WAIT_DATA_BEGIN;
                session->retry_count_            = 0;
                YmodemReceiveSessionFileHandleBegin(session, data, size);
            } else if (msg_type == YMODEM_MESSAGE_TYPE_128_NUL_BLOCK) {
                YmodemReceiveSessionCancle(session);
                session->result_ = YMODEM_SESSION_RESULT_ERROR;
            } else {
                // Nothing to do
            }
            break;
        case YMODEM_RECEIVE_WAIT_DATA_BEGIN:
            msg_type = msg_type == YMODEM_MESSAGE_TYPE_128_NUL_BLOCK ? YMODEM_MESSAGE_TYPE_128_BLOCK : msg_type;
            if (msg_type == YMODEM_MESSAGE_TYPE_128_BLOCK && session->next_expected_sequence_ == sequence) {
                session->next_expected_sequence_ = sequence + 1;
                session->state_                  = YMODEM_RECEIVE_WAIT_DATA;
                session->retry_count_            = 0;
                const uint8_t ack                = ACK;
                session->write(&ack, 1);
                YmodemPushDataToFile(session, data, size);
            } else if (msg_type == YMODEM_MESSAGE_TYPE_1K_BLOCK && session->next_expected_sequence_ == sequence) {
                session->next_expected_sequence_ = sequence + 1;
                session->state_                  = YMODEM_RECEIVE_WAIT_DATA;
                session->retry_count_            = 0;
                const uint8_t ack                = ACK;
                session->write(&ack, 1);
                YmodemPushDataToFile(session, data, size);
            } else if (msg_type == YMODEM_MESSAGE_TYPE_CMD && cmd == EOT) {
                session->state_       = YMODEM_RECEIVE_WAIT_SECOND_EOT;
                session->retry_count_ = 0;
            } else {
                // Nothing to do
            }
            break;
        case YMODEM_RECEIVE_WAIT_DATA:
            msg_type = msg_type == YMODEM_MESSAGE_TYPE_128_NUL_BLOCK ? YMODEM_MESSAGE_TYPE_128_BLOCK : msg_type;
            if (msg_type == YMODEM_MESSAGE_TYPE_128_BLOCK && session->next_expected_sequence_ == sequence) {
                session->retry_count_            = 0;
                session->next_expected_sequence_ = sequence + 1;
                const uint8_t ack                = ACK;
                session->write(&ack, 1);
                YmodemPushDataToFile(session, data, size);
            } else if (msg_type == YMODEM_MESSAGE_TYPE_1K_BLOCK && session->next_expected_sequence_ == sequence) {
                session->retry_count_            = 0;
                session->next_expected_sequence_ = sequence + 1;
                const uint8_t ack                = ACK;
                session->write(&ack, 1);
                YmodemPushDataToFile(session, data, size);
            } else if (msg_type == YMODEM_MESSAGE_TYPE_CMD && cmd == EOT) {
                session->state_       = YMODEM_RECEIVE_WAIT_SECOND_EOT;
                session->retry_count_ = 0;
                bool ret              = YmodemFileClose(&session->file);
                if (ret) {
                    session->result_ = YMODEM_SESSION_RESULT_OK;
                } else {
                    session->result_ = YMODEM_SESSION_RESULT_ERROR;
                }
            } else {
                YmodemReceiveSessionCancle(session);
                session->retry_count_ = 0;
                session->state_       = YMODEM_SESSION_CLOSED;
                session->result_      = YMODEM_SESSION_RESULT_ERROR;
            }
            break;
        case YMODEM_RECEIVE_WAIT_SECOND_EOT:
            if (msg_type == YMODEM_MESSAGE_TYPE_CMD && cmd == EOT) {
                session->state_       = YMODEM_RECEIVE_WAIT_NEXT_FILE_OR_WAIT_END;
                session->retry_count_ = 0;
            } else {
                // Nothing to do
            }
            break;
        case YMODEM_RECEIVE_WAIT_NEXT_FILE_OR_WAIT_END:
            if (msg_type == YMODEM_MESSAGE_TYPE_128_NUL_BLOCK) {
                session->state_       = YMODEM_SESSION_CLOSED;
                session->retry_count_ = 0;
                const uint8_t ack     = ACK;
                session->write(&ack, 1);
                session->result_ = YMODEM_SESSION_RESULT_OK;
            } else if (msg_type == YMODEM_MESSAGE_TYPE_128_BLOCK && sequence == 0) {
                session->next_expected_sequence_ = sequence + 1;
                session->state_                  = YMODEM_RECEIVE_WAIT_DATA_BEGIN;
                session->retry_count_            = 0;
                YmodemReceiveSessionFileHandleBegin(session, data, size);
            } else {
                // Nothing to do
            }
            break;
        default:
            break;
    }

    return session->result_;
}

void YmodemSessionPoll(YmodemSession_t *session)
{
    if (session == NULL) {
        return;
    }

    if (session->state_ == YMODEM_SESSION_CLOSED) {
        return;
    }

    size_t len = 0;
    len        = session->read(session->buffer_ + session->buffer_size_, YMODEM_FRAME_SIZE_1K - session->buffer_size_);
    if (len > 0) {
        session->buffer_size_ += len;
        session->last_read_tick_ms_ = YMODEM_GET_MS_TICK();
    }

    if (YMODEM_GET_MS_TICK() - session->last_read_tick_ms_ > 10 && session->buffer_size_ > 0) {
        // 10ms内没有读到数据，且缓冲区有数据，视为一条消息
        YmodemPushMessageToReceiveSession(session, session->buffer_, session->buffer_size_);
        session->buffer_size_ = 0;
    }

    if (session->file_is_open_ == true && session->result_ != YMODEM_SESSION_RESULT_NONE) {
        YmodemFileClose(&session->file);
        session->file_is_open_ = false;
    }

    // 确保第一次发送在开始会话后直接开始
    // 接收状态切换时session->retry_count_会被清零
    if (YMODEM_GET_MS_TICK() - session->last_send_message_tick_ms_ > session->timeout_ || session->retry_count_ == 0) {
        if (session->retry_count_ > session->retry_max_count_) {
            YmodemReceiveSessionCancle(session);
            session->result_ = YMODEM_SESSION_RESULT_TIMEOUT;
            session->state_  = YMODEM_SESSION_CLOSED;

            if (session->file_is_open_) {
                YmodemFileClose(&session->file);
                if (YmodemIsFileExist(session->file_name_)) {
                    // 文件存在，删除
                    YmodemFileRemove(session->file_name_);
                }
                session->file_is_open_ = false;
            }
            return;
        }

        session->retry_count_++;
        session->last_send_message_tick_ms_ = YMODEM_GET_MS_TICK();
        // 按照协议最多有两个控制字符连续发送
        uint8_t cmds_buf[2] = {0};
        switch (session->state_) {
            case YMODEM_RECEIVE_WAIT_HEADER:
                cmds_buf[0] = CNC;
                session->write(cmds_buf, 1);
                break;
            case YMODEM_RECEIVE_WAIT_DATA_BEGIN:
                cmds_buf[0] = ACK;
                cmds_buf[1] = CNC;
                session->write(cmds_buf, 2);
                break;
            case YMODEM_RECEIVE_WAIT_DATA:
                break;
            case YMODEM_RECEIVE_WAIT_SECOND_EOT:
                cmds_buf[0] = NAK;
                session->write(cmds_buf, 1);
                break;
            case YMODEM_RECEIVE_WAIT_NEXT_FILE_OR_WAIT_END:
                cmds_buf[0] = ACK;
                cmds_buf[1] = CNC;
                session->write(cmds_buf, 2);
                break;
            default:
                break;
        }
    }
}
