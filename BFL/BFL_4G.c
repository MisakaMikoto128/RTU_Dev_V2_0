/**
 * @file BFL_4G.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-09-17
 * @last modified 2023-09-17
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "BFL_4G.h"
#include "cqueue.h"
#include "log.h"
#include "BFL_4G_Task.h"

#define SOCKET_BUF_SIZE 512
AsyncTaskExecContext_t context;

void BFL_4G_Init(const char *PDP_type, const char *APN)
{
    CommunPara_t *pCommunPara = &context.CommunPara;
    if (APN != NULL)
    {
        strncpy(pCommunPara->APN, APN, 5);
    }
    else
    {
        strncpy(pCommunPara->APN, "CMIOT", 5);
    }

    if (PDP_type != NULL)
    {
        strncpy(pCommunPara->PDP_type, PDP_type, 6);
    }
    else
    {
        strncpy(pCommunPara->PDP_type, "IP", 6);
    }

    BFL_4G_AsyncTaskExecContext_Init();
}

/**
 * @brief
 *
 * @param batch
 * @param task
 */
int32_t BFL_4G_TCP_Init(int sockid, const char *hostAddr, uint16_t port)
{
    CommunPara_t *pCommunPara = &context.CommunPara;
    strncpy(pCommunPara->hostAddr[sockid], hostAddr, 32);
    pCommunPara->socketPorts[sockid] = port;

    if (pCommunPara->socketIsEnable[sockid] == false)
    {
        pCommunPara->socketIsEnable[sockid] = true;
        BFL_4G_TCP_List_TaskCreate(sockid);

        // TODO:urc table
        BFL_4G_TCP_Task_UCRTable_Init(sockid);
        context.socketRevBufs[sockid] = (uint8_t *)at_malloc(SOCKET_BUF_SIZE);
        cqueue_create(&context.socketRevQueues[sockid], context.socketRevBufs[sockid], SOCKET_BUF_SIZE, sizeof(uint8_t));
    }
    return 0;
}

uint32_t BFL_4G_TCP_Write(int sockid, uint8_t *writeBuf, uint32_t uLen)
{
    uint32_t ret = 0;
    ret = BFL_4G_TCPWrite_Task(sockid, writeBuf, uLen);
    return ret;
}

uint32_t BFL_4G_TCP_Writeable(int sockid)
{
    UNUSED(sockid);
    uint32_t ret = 0;
    bool result = false;
    result = BFL_4G_TCP_Task_Writeable();
    ret = (uint32_t)result;
    return ret;
}

uint32_t BFL_4G_TCP_Read(int sockid, unsigned char *pBuf, uint32_t uiLen)
{
    uint32_t ret = 0;
    ret = cqueue_out(&context.socketRevQueues[sockid], pBuf, uiLen);
    return ret;
}

uint32_t BFL_4G_TCP_Readable(int sockid)
{
    return cqueue_size(&context.socketRevQueues[sockid]);
}

void BFL_4G_Poll()
{
    at_obj_process(context.at_obj);
    AsyncTaskList_Exec(context.task_list);
}

void BFL_4G_SetCalibrateTimeByUtcSecondsCb(void (*setCalibrateTimeByUtcSecondsCb)(uint64_t utcSeconds))
{
    context.setCalibrateTimeByUtcSecondsCb = setCalibrateTimeByUtcSecondsCb;
}

void BFL_4G_StartCalibrateTimeOneTimes()
{
    BFL_4G_StartCalibrateTimeOneTimes_Task();
}