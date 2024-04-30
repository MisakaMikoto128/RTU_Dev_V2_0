/**
 * @file BFL_4G_Task.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-09-21
 * @last modified 2023-09-21
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef BFL_4G_TASK_H
#define BFL_4G_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "at_chat.h"
#include "cqueue.h"
#include "AsyncTaskList.h"

#define ALLOWED_PUBLIC_TOPIC_NUM    1
#define ALLOWED_SUBSCRIBE_TOPIC_NUM 4

/*
可用订阅ID: 0-4
可用发布ID: 0-4
可用sockid: 现在无
*/
typedef struct
{
    char APN[5 + 1]; // APN,如果为NULL将使用默认的APN,CMIOT
    char PDP_type[6 + 1];

    char hostName[128 + 1];                                // 主机名
    uint32_t port;                                         // 端口号
    char clientID[128 + 1];                                // 客户端ID
    uint32_t keepAliveInterval;                            /* 保持连接时间（s）。  */
    char userName[128 + 1];                                // 用户名
    char password[128 + 1];                                // 密码
    uint8_t cleanSession;                                  /* 是否清除会话（0-1），是否删除会话。默认不删除0*/
    uint8_t encrypt;                                       /* 是否加密（0-1）。默认不加密0*/
    char subscribeTopic[ALLOWED_SUBSCRIBE_TOPIC_NUM][512]; // 订阅列表，在数组中的位置对于topicID，未使用的topicID需要初始化为NULL,如果对应位置之前配置过了，那么重新配置为NULL会取消订阅。
    uint8_t subscribeTopicQos[ALLOWED_SUBSCRIBE_TOPIC_NUM];
    char publishTopic[ALLOWED_PUBLIC_TOPIC_NUM][512]; // 发布列表，在数组中的位置对于topicID，未使用的topicID需要初始化为NULL
    uint8_t publishTopicQos[ALLOWED_PUBLIC_TOPIC_NUM];
    uint8_t publishTopicRetain[ALLOWED_PUBLIC_TOPIC_NUM];

    char hostAddr[3][32 + 1]; // socket连接的服务器地址
    uint16_t socketPorts[3];  // socket连接的服务器地址
    bool socketIsEnable[3];   ////socket是否使能
} CommunPara_t;

typedef struct AsyncTaskExecContext {
    CommunPara_t CommunPara;
    at_obj_t *at_obj;
    bool isIdel;
    bool base_cfg_ok; // 参数是否设置了
    uint8_t *write_buf;
    uint32_t write_buf_len;
    uint8_t writeIsUsing; // write Buffer 是否被占用
    bool baseCfgIsOk;     // 基本配置过程实际执行完成
    uint8_t *socketRevBufs[3];
    CQueue_t socketRevQueues[3];
    AsyncTask_t *sockeOpenTasks[3];
    urc_item_t urc_table[3]; // URC table
    uint8_t urc_table_size;

    void (*setCalibrateTimeByUtcSecondsCb)(uint64_t utcSeconds);

    // 全局计数器
    int qiopen_fail_times;
    int socket_send_fail_times[3];

    //
    AsyncTaskList_t *task_list;
} AsyncTaskExecContext_t;

void BFL_4G_AsyncTaskExecContext_Init();
void BFL_4G_BaseCfgTaskCreate();
void BFL_4G_IOInterfaceInit();
void BFL_4G_TCP_TaskCreate(int sockid);
uint32_t BFL_4G_TCPWrite_Task(int sockid, uint8_t *writeBuf, uint32_t uLen);
bool BFL_4G_TCP_Task_Writeable();
void BFL_4G_TCP_Task_UCRTable_Init(int sockid);
void BFL_4G_StartCalibrateTimeOneTimes_Task();

void BFL_4G_List_BaseCfgTaskCreate();
void BFL_4G_TaskList_Create(int sockid);
#ifdef __cplusplus
}
#endif
#endif //! BFL_4G_TASK_H
