/**
 * @file Ymodem.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-05-26
 * @last modified 2023-05-26
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef YMODEM_H
#define YMODEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ymodem_if.h"
#define YMODEM_FRAME_HEADER_LEN (3)
#define YMODEM_FRAME_CHECK_LEN  (2)
#define YMODEM_FRAME_SIZE_128   (128 + (YMODEM_FRAME_HEADER_LEN + YMODEM_FRAME_CHECK_LEN))
#define YMODEM_FRAME_SIZE_1K    (1024 + (YMODEM_FRAME_HEADER_LEN + YMODEM_FRAME_CHECK_LEN))
/*
词汇解释：
数据包block：就是两种数据帧。
消息message 指的是Ymodem通信过程中的两种数据帧和所有命令。
*/

/**
 * @brief Ymodem会话状态。
 *
 */
typedef enum eYmodemSessionState_t {
    /*
    发送/接收会话关闭
    */
    YMODEM_SESSION_CLOSED,
    /*
    等待接收非空首部block0状态,此时接收会话开始,会一直发送'C'，直到收到block0或者超时
    值得注意的是YMODEM的协议文档《XMODEM/YMODEM PROTOCOL REFERENCE》中这个block0只包含了文件名和，多余的部分使用NUL填充，
    这样在传输文本文件时没有问题，但是如果传输二进制文件，就没办法区分哪些是文件内容，哪些是填充的CPMEOF，所以这里的block0
    数据部分包含了: 文件名NUL文件大小NUL[]。取消认为CPMEOF是文件结尾。
    */
    YMODEM_RECEIVE_WAIT_HEADER,
    /* 等待接收到第一个数据块block1,接收会话会一直发送ACK、'C'，直到收到block1或者超时 */
    YMODEM_RECEIVE_WAIT_DATA_BEGIN,
    /*
    等待接收数据块，接收会话会一直发送ACK，直到收到block_i(i >= 1 && i <= 255)或者超时。此时如果收到EOT，则进入等待第二个EOT状态。
    此时如果由于文件比较大，编号超过了255，接下来的数据块编号会从0开始重新计数，这是的block0是一个包含文件数据的数据包，而不是文件信息。
    此时如果发送的文件是个空文件，这种情况在ymodem协议文档中没有明确说明，但是因为存在两次EOT，所以不管发送发是先发送一个全为CPMEOF的数据包
    还是直接发送两个EOT，接收方都可以实现正确的判断接收结束。
    */
    YMODEM_RECEIVE_WAIT_DATA,
    // 等待接收第二个EOT状态，接收会话会一直发送ACK、'C'，直到收到EOT或者超时。此时如果收到EOT，则进入等待结束或者下一个文件的状态。
    YMODEM_RECEIVE_WAIT_SECOND_EOT,
    /*
    等待结束或者下一个文件的状态。此时接收会话会一直发送ACK、'C'，直到收到空block0或者超时。
    此时如果收到空block0，则发送ACK，结束会话。
    此时如果收到非空block0，则进入等待接收非空首部block0状态。
    */
    YMODEM_RECEIVE_WAIT_NEXT_FILE_OR_WAIT_END,
} YmodemSessionState_t;

typedef enum eYmodemSessionResult_t {
    YMODEM_SESSION_RESULT_NONE,    // 还没有结果
    YMODEM_SESSION_RESULT_OK,      // 成功
    YMODEM_SESSION_RESULT_TIMEOUT, // 所有重试均超时
    YMODEM_SESSION_RESULT_ERROR,   // 发生错误
    YMODEM_SESSION_RESULT_CANCLE,  // 取消
} YmodemSessionResult_t;

typedef struct eYmodemSession_t {
    uint32_t timeout_;                   // 单次等待回复超时时间，单位毫秒
    uint32_t last_send_message_tick_ms_; // 时间戳，单位毫秒
    uint32_t next_expected_sequence_;    // 当前数据块编号
    uint8_t retry_count_;                // 当前重试次数
    uint8_t retry_max_count_;            // 最大重试次数
    uint8_t received_cancle_count_;      // 是否收到了取消命令
    YmodemSessionState_t state_;         // 会话状态
    YmodemSessionResult_t result_;       // 会话结果

    uint8_t buffer_[(1024 + (YMODEM_FRAME_HEADER_LEN + YMODEM_FRAME_CHECK_LEN))]; // 数据缓冲区
    uint32_t buffer_size_;                                                        // 数据缓冲区当前元素个数
    uint32_t last_read_tick_ms_;                                                  // 时间戳，单位毫秒
    size_t (*read)(uint8_t *buffer, size_t size);                                 // 读数据
    size_t (*write)(const uint8_t *buffer, size_t size);                          // 写数据

    char file_name_[YMODEM_FRAME_SIZE_128 + 1]; // 文件名
    uint32_t file_size_;                        // 文件大小,单位字节
    uint32_t file_offset_;                      // 文件偏移量
    YmodemFile_t file;                          // 文件指针
    bool file_is_open_;                         // 文件是否打开
} YmodemSession_t;

/**
 * @brief 打开一个发送会话。
 *
 * @param session
 * @return true 打开成功。
 * @return false 打开失败。
 */
bool YmodemReceiveSessionOpen(YmodemSession_t *session, size_t (*read)(uint8_t *buf, size_t size), size_t (*write)(const uint8_t *buf, size_t size));
void YmodemReceiveSessionCancle(YmodemSession_t *session);
void YmodemReceiveSessionInit(YmodemSession_t *session, size_t (*read)(uint8_t *buf, size_t size), size_t (*write)(const uint8_t *buf, size_t size));
/**
 * @brief 时间轮询，负责处理超时重试。
 *
 * @param session
 */
void YmodemSessionPoll(YmodemSession_t *session);

YmodemSessionResult_t YmodemSessionGetResult(YmodemSession_t *session);
void YmodemSessionClearResult(YmodemSession_t *session);

typedef enum {
    YMODEM_MESSAGE_TYPE_128_BLOCK,     // 128字节的数据块
    YMODEM_MESSAGE_TYPE_128_NUL_BLOCK, // 128字节的空数据块
    YMODEM_MESSAGE_TYPE_1K_BLOCK,      // 1K字节的数据块
    YMODEM_MESSAGE_TYPE_CMD,           // 单个命令
    YMODEM_MESSAGE_TYPE_CMDS,          // 多个命令
    YMODEM_MESSAGE_TYPE_UNKNOWN,       // 未知类型
} YmodemMessageType_t;

#ifdef __cplusplus
}
#endif
#endif //! YMODEM_H
