/**
 * @file BFL_4G_Task.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-09-21
 * @last modified 2023-09-21
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#include "cqueue.h"
#include <stdlib.h>
#include "at_chat.h"
#include <string.h>
#include "log.h"
#include "BFL_4G_Task.h"
#include "BFL_4G.h"
#include <stdio.h>
#include "mtime.h"
#include "HDL_G4_Uart.h"
#include "CHIP_EC800M.h"
#ifdef FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#endif // FREE_RTOS

// Lock, used in OS environment, fill in NULL if not required.
void at_device_lock(void)
{
#ifdef FREE_RTOS
    // vTaskSuspendAll();
#endif // FREE_RTOS
}

// Unlock, used in OS environment, fill in NULL if not required.
void at_device_unlock(void)
{
#ifdef FREE_RTOS
    // xTaskResumeAll();
#endif // FREE_RTOS
}

/**
 * @brief 数据写操作
 * @param buf  数据缓冲区
 * @param size 缓冲区长度
 * @retval 实际写入数据
 */
unsigned int at_device_write(const void *buf, unsigned int size)
{
    return Uart_Write(CHIP_EC800M_COM, buf, size);
}
/**
 * @brief 数据读操作
 * @param buf  数据缓冲区
 * @param size 缓冲区长度
 * @retval 实际读到的数据
 */
unsigned int at_device_read(void *buf, unsigned int size)
{
    return Uart_Read(CHIP_EC800M_COM, buf, size);
}

typedef void (*debug_t)(const char *fmt, ...);
int32_t AsyncTaskFuncResultMap(at_resp_code code);
/**
 * @brief AT适配器
 */
static const at_adapter_t at_adapter = {
    .lock = NULL,                   // 多任务上锁(非OS下填NULL)
    .unlock = NULL,                 // 多任务解锁(非OS下填NULL)
    .write = at_device_write,       // 数据写接口
    .read = at_device_read,         // 数据读接口)
    .debug = (debug_t)Debug_Printf, // 调试打印接口 NULL,//
    .urc_bufsize = 1 * 1024,
    .recv_bufsize = 1 * 1024, // 接收缓冲区大小
};

#define AT_BUF_LEN 1024
static uint8_t at_output_buf[AT_BUF_LEN];
extern AsyncTaskExecContext_t context;

void at_task_delay(uint32_t delayMs);
void at_task_send();
void at_cpin_q_task_send();
void at_creg_q_task_send();
void at_cfun_task_send();
void at_cgdcont_task_send();
void at_cgact_task_send();
void at_qicsgp_task_send();
void at_qicfg_task_send();
void at_clk_task_send();

void at_qisde_task_send(int sockid);
void at_qiopen_task_send(int sockid);
void at_qiswtmd_task_send(int sockid);
void at_qisend_task_send(int sockid);

void at_task_delay(uint32_t delayMs)
{
    at_attr_t attr;

    attr.params = NULL;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = NULL;
    attr.timeout = delayMs;
    attr.retry = 0;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_send_singlline(context.at_obj, &attr, "");
}

void at_task_delay_until(uint32_t delayMs, const char *suffix)
{
    at_attr_t attr;

    attr.params = NULL;
    attr.prefix = NULL;
    attr.suffix = suffix;
    attr.cb = NULL;
    attr.timeout = delayMs;
    attr.retry = 0;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_send_singlline(context.at_obj, &attr, "");
}

void at_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK || r->code == AT_RESP_ERROR)
    {
        ULOG_INFO("[4G] AT  R OK!正确开机。");
    }
    else
    {
        ULOG_INFO("[4G] AT 指令响应失败。");
    }
}

void at_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = at_callback;
    attr.timeout = 1000;
    attr.retry = 2;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_send_singlline(context.at_obj, &attr, "AT");
}

void at_cpin_q_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK)
    {
        ULOG_INFO("[4G] AT+CPIN? R OK! SIM卡状态正常");
    }
    else
    {
        ULOG_INFO("[4G] AT+CPIN? R OK! SIM卡状态异常");
    }
}

void at_cpin_q_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = "+CPIN:";
    attr.suffix = "READY";
    attr.cb = at_cpin_q_callback;
    attr.timeout = 1000;
    attr.retry = 2;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_exec_cmd(context.at_obj, &attr, "AT+CPIN?");
}

void at_creg_q_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK)
    {
        ULOG_INFO("[4G] AT+CREG? R OK! 网络注册成功");
    }
    else
    {
        ULOG_INFO("[4G] AT+CREG?R FAIL! 网络注册失败");
    }
}

void at_creg_q_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = "+CREG:";
    attr.suffix = "OK";
    attr.cb = at_creg_q_callback;
    attr.timeout = 1000;
    attr.retry = 2;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_send_singlline(context.at_obj, &attr, "AT+CREG?");
}

void at_cfun_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK)
    {
        ULOG_INFO("[4G] AT+CFUN=1 R OK!");
    }
    else
    {
        ULOG_INFO("[4G] AT+CFUN=1 R FAIL!");
    }
}

void at_cfun_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = at_cfun_callback;
    attr.timeout = 1000;
    attr.retry = 2;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_exec_cmd(context.at_obj, &attr, "AT+CFUN=1");
}

void at_cgdcont_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK)
    {
        ULOG_INFO("[4G] AT+CGDCONT=1,%s,%s R OK!", context.CommunPara.PDP_type, context.CommunPara.APN);
    }
    else
    {
        ULOG_INFO("[4G] AT+CGDCONT=1 R FAIL!");
    }
}

void at_cgdcont_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = at_cgdcont_callback;
    attr.timeout = 1000;
    attr.retry = 2;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_exec_cmd(context.at_obj, &attr, "AT+CGDCONT=1,%s,%s", context.CommunPara.PDP_type, context.CommunPara.APN);
}

void at_cgact_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = NULL;
    attr.timeout = 1000;
    attr.retry = 2;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    at_exec_cmd(context.at_obj, &attr, "AT+CGACT=1,1");
}

void at_qicsgp_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK)
    {
        ULOG_INFO("[4G] AT+QICSGP=1,1,\"%s\",\"\",\"\",1 R OK!", context.CommunPara.APN);
    }
    else
    {
        ULOG_INFO("[4G] AT+QICSGP=1,1,\"%s\",\"\",\"\",1 R FAIL!", context.CommunPara.APN);
    }
}

void at_qicsgp_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = at_qicsgp_callback;
    attr.timeout = 2000;
    attr.retry = 1;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_exec_cmd(context.at_obj, &attr, "AT+QICSGP=1,1,\"%s\",\"\",\"\",1", context.CommunPara.APN);
    at_task_delay(500);
}

void at_qicfg_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK)
    {
        ULOG_INFO("[4G] AT+QICFG %s R OK!", r->recvbuf);
    }
    else
    {
        ULOG_INFO("[4G] AT+QICFG %s R FAIL!", r->recvbuf);
    }
}

void at_qicfg_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = at_qicfg_callback;
    attr.timeout = 1000;
    attr.retry = 3;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    static const char *cmds[] = {
        "AT+QIACT=1",
        "AT+QICFG=\"passiveclosed\",1",
        "AT+QICFG=\"tcp/keepalive\",1,240,60,3",
        "AT+QICFG=\"tcp/retranscfg\",5,10",
        "AT+QICFG=\"send/auto\",1,30,\"AA\"",
        "AT+QICFG=\"qisend/timeout\",2",
        "AT+QICFG=\"TCP/SendMode\",0",
        "AT+QNTP=1,\"ntp.ntsc.ac.cn\"",
        "AT+QSCLKEX=1,1,10",
        "AT+QICFG=\"dataformat\",0,1",
        NULL};
    at_send_multiline(context.at_obj, &attr, cmds);
}

void at_qicfg_dataformat_task1_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = at_qicfg_callback;
    attr.timeout = 1000;
    attr.retry = 3;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    at_send_singlline(context.at_obj, &attr, "AT+QIACT=1");
    at_send_singlline(context.at_obj, &attr, "AT+QICFG=\"dataformat\",0,1");
    at_send_singlline(context.at_obj, &attr, "AT+QICLOSE=0");
}

void at_qicfg_close_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = NULL;
    attr.timeout = 1000;
    attr.retry = 3;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    at_send_singlline(context.at_obj, &attr, "AT+QICLOSE=0");
}

void at_qicfg_dataformat_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = NULL;
    attr.timeout = 1000;
    attr.retry = 3;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    at_send_singlline(context.at_obj, &attr, "AT+QICFG=\"dataformat\",0,1");
}

void at_clk_callback(at_response_t *r)
{
    if (r->code == AT_RESP_OK)
    {

        /*
        AT+CCLK?

        +CCLK: "23/08/07,07:06:54+32"

        OK
        */

        char time[32] = {0};
        sscanf(r->prefix, "+CCLK: \"%[^\"]", time);

        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;
        char timezoneSign = 0;
        int timezone = 0;
        sscanf(time, "%02d/%02d/%02d,%02d:%02d:%02d%c%02d", &year, &month, &day, &hour, &minute, &second, &timezoneSign, &timezone);
        if (timezoneSign == '-')
        {
            timezone = -timezone;
        }
        timezone = timezone / 4;

        mtime_t mtime;
        mtime.nYear = year + 2000;
        mtime.nMonth = month;
        mtime.nDay = day;
        mtime.nHour = hour;
        mtime.nMin = minute;
        mtime.nSec = second;

        uint64_t utcTimestamp = 0;
        utcTimestamp = mtime_2_utc_sec(&mtime);

        ULOG_INFO("[4G] %llu->%04d.%02d.%02d %02d:%02d:%02d", utcTimestamp, mtime.nYear, mtime.nMonth, mtime.nDay, mtime.nHour, mtime.nMin, mtime.nSec);
        if (context.setCalibrateTimeByUtcSecondsCb != NULL)
        {
            context.setCalibrateTimeByUtcSecondsCb(utcTimestamp);
        }
    }
    else
    {
        ULOG_INFO("[4G] 获取时间失败");
    }
}

void at_clk_task_send()
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = "+CCLK: ";
    attr.suffix = "OK";
    attr.cb = at_clk_callback;
    attr.timeout = 1000;
    attr.retry = 6;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    at_send_singlline(context.at_obj, &attr, "AT+CCLK?");
}

void at_qisde_task_send(int sockid)
{
    at_attr_t attr;

    attr.params = &context;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = NULL;
    attr.timeout = 1000;
    attr.retry = 2;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    at_exec_cmd(context.at_obj, &attr, "AT+QISDE=%d", sockid);
}

void at_qisde_task_send_(void *param)
{
    int sockid = (int)param;
    at_qisde_task_send(sockid);
}

void at_qiopen_callback(at_response_t *r)
{
    uint8_t *params_tuple = (uint8_t *)r->params;
    AsyncTaskExecContext_t *pContext = *(AsyncTaskExecContext_t **)params_tuple;
    params_tuple += sizeof(AsyncTaskExecContext_t *);
    int sockid = *(int *)params_tuple;
    at_free(r->params);
    if (r->code == AT_RESP_OK)
    {
        ULOG_INFO("[4G] TCP连接成功");
        context.qiopen_fail_times = 0;
    }
    else
    {
        ULOG_INFO("[4G] TCP连接失败");
        at_task_delay(1000);
        at_qiopen_task_send(sockid);

        context.qiopen_fail_times++;
        ULOG_ERROR("[4G] TCP连接失败次数:%d", context.qiopen_fail_times);
        if (context.qiopen_fail_times > 3)
        {
            ULOG_ERROR("[4G] TCP连接失败次数过多，重新连接");
            context.qiopen_fail_times = 0;
        }
    }
}

void at_qiopen_task_send(int sockid)
{
    at_attr_t attr;
    void *params = at_malloc(sizeof(AsyncTaskExecContext_t *) + sizeof(int));
    uint8_t *params_tuple = (uint8_t *)params;
    *(AsyncTaskExecContext_t **)params_tuple = &context;
    params_tuple += sizeof(AsyncTaskExecContext_t *);
    *(int *)params_tuple = sockid;

    attr.params = params;
    attr.prefix = "OK";
    attr.suffix = "+QIOPEN";
    attr.cb = at_qiopen_callback;
    attr.timeout = 3000;
    attr.retry = 1;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    at_exec_cmd(context.at_obj, &attr, "AT+QIOPEN=1,%d,\"%s\",\"%s\",%d,0,0", sockid, "TCP", context.CommunPara.hostAddr[sockid], context.CommunPara.socketPorts[sockid]);
}

void at_qiopen_task_send_(void *param)
{
    int sockid = (int)param;
    at_qiopen_task_send(sockid);
}

void at_qiswtmd_callback(at_response_t *r)
{
    uint8_t *params_tuple = (uint8_t *)r->params;
    AsyncTaskExecContext_t *pContext = *(AsyncTaskExecContext_t **)params_tuple;
    params_tuple += sizeof(AsyncTaskExecContext_t *);
    int sockid = *(int *)params_tuple;
    at_free(r->params);
    if (r->code == AT_RESP_OK)
    {
        ULOG_INFO("[4G] TCP设置为直吐模式");
        context.writeIsUsing = 0;
    }
    else
    {
        ULOG_INFO("[4G] TCP设置为直吐模式失败");
    }
}

void at_qiswtmd_task_send(int sockid)
{
    at_attr_t attr;
    void *params = at_malloc(sizeof(AsyncTaskExecContext_t *) + sizeof(int));
    uint8_t *params_tuple = (uint8_t *)params;
    *(AsyncTaskExecContext_t **)params_tuple = &context;
    params_tuple += sizeof(AsyncTaskExecContext_t *);
    *(int *)params_tuple = sockid;

    attr.params = params;
    attr.prefix = NULL;
    attr.suffix = "OK";
    attr.cb = at_qiswtmd_callback;
    attr.timeout = 1000;
    attr.retry = 1;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;
    at_exec_cmd(context.at_obj, &attr, "AT+QISWTMD=%d,1", sockid);
}

void at_qiswtmd_task_send_(void *param)
{
    int sockid = (int)param;
    at_qiswtmd_task_send(sockid);
}

void at_qisend_callback(at_response_t *r)
{
    uint8_t *params_tuple = (uint8_t *)r->params;
    AsyncTaskExecContext_t *pContext = *(AsyncTaskExecContext_t **)params_tuple;
    params_tuple += sizeof(AsyncTaskExecContext_t *);
    int sockid = *(int *)params_tuple;
    at_free(r->params);

    if (r->code == AT_RESP_OK)
    {
        context.writeIsUsing = 0;
        pContext->socket_send_fail_times[sockid] = 0;
        ULOG_INFO("[4G] TCP发送成功!");
    }
    else
    {
        if (pContext->socket_send_fail_times[sockid] > 3)
        {
            ULOG_ERROR("[4G] TCP发送失败次数过多，重新连接");
            pContext->socket_send_fail_times[sockid] = 0;
        }
        else
        {
            pContext->socket_send_fail_times[sockid]++;
            ULOG_ERROR("[4G] TCP发送失败次数:%d", pContext->socket_send_fail_times[sockid]);
        }
    }
}

/**
 * @brief
 *
 * @param sockid
 */
void at_qisend_task_send(int sockid)
{
    at_attr_t attr;

    void *params = at_malloc(sizeof(AsyncTaskExecContext_t *) + sizeof(int));
    uint8_t *params_tuple = (uint8_t *)params;
    *(AsyncTaskExecContext_t **)params_tuple = &context;
    params_tuple += sizeof(AsyncTaskExecContext_t *);
    *(int *)params_tuple = sockid;

    attr.params = params;
    attr.prefix = NULL;
    attr.suffix = "SEND OK";
    attr.cb = at_qisend_callback;
    attr.timeout = 3000;
    attr.retry = 3;
    attr.priority = AT_PRIORITY_LOW;
    attr.ctx = NULL;

    at_send_singlline(context.at_obj, &attr, (char *)context.write_buf);
}

void at_qisend_task_send_(void *param)
{
    int sockid = (int)param;
    at_qisend_task_send(sockid);
}

void BFL_4G_BaseCfgTaskCreate()
{
    // at_task_delay(5000);
    at_task_send();
    at_cpin_q_task_send();
    // at_task_delay(3000);
    at_creg_q_task_send();
    at_cfun_task_send();
    at_cgdcont_task_send();
    // at_task_delay(6000);

    at_cgact_task_send();
    at_qicsgp_task_send();
    at_qicfg_task_send();
    at_clk_task_send();
}

AsyncTask_t *at_task;
AsyncTask_t *at_cpin_q_task;
AsyncTask_t *at_creg_q_task;
AsyncTask_t *at_cfun_task;
AsyncTask_t *at_cgdcont_task;
AsyncTask_t *at_cgact_task;
AsyncTask_t *at_qicsgp_task;
AsyncTask_t *at_qicfg_task;
AsyncTask_t *at_clk_task;
AsyncTask_t *at_qicfg_close_task;
AsyncTask_t *at_qicfg_dataformat_task;

void BFL_4G_List_BaseCfgTaskCreate()
{
    AsyncTaskList_t *task_list = context.task_list;
    at_task = AsyncTask_Create(at_task_send, NULL, 5000);
    at_cpin_q_task = AsyncTask_Create(at_cpin_q_task_send, NULL, 0);
    at_creg_q_task = AsyncTask_Create(at_creg_q_task_send, NULL, 0);
    at_cfun_task = AsyncTask_Create(at_cfun_task_send, NULL, 0);
    at_cgdcont_task = AsyncTask_Create(at_cgdcont_task_send, NULL, 0);
    at_cgact_task = AsyncTask_Create(at_cgact_task_send, NULL, 0);
    at_qicsgp_task = AsyncTask_Create(at_qicsgp_task_send, NULL, 0);
    at_qicfg_task = AsyncTask_Create(at_qicfg_task_send, NULL, 0);
    at_clk_task = AsyncTask_Create(at_clk_task_send, NULL, 0);
    at_qicfg_close_task = AsyncTask_Create(at_qicfg_close_task_send, NULL, 0);
    at_qicfg_dataformat_task = AsyncTask_Create(at_qicfg_dataformat_task_send, NULL, 0);

    AsyncTaskList_StaticAdd(task_list, at_task);
    AsyncTaskList_StaticAdd(task_list, at_cpin_q_task);
    AsyncTaskList_StaticAdd(task_list, at_creg_q_task);
    AsyncTaskList_StaticAdd(task_list, at_cfun_task);
    AsyncTaskList_StaticAdd(task_list, at_cgdcont_task);
    AsyncTaskList_StaticAdd(task_list, at_cgact_task);
    AsyncTaskList_StaticAdd(task_list, at_qicsgp_task);
    AsyncTaskList_StaticAdd(task_list, at_qicfg_task);
    AsyncTaskList_StaticAdd(task_list, at_clk_task);
    AsyncTaskList_StaticAdd(task_list, at_qicfg_close_task);
    AsyncTaskList_StaticAdd(task_list, at_qicfg_dataformat_task);

    AsyncTask_SetRoute(at_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_task);
    AsyncTask_SetRoute(at_cpin_q_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_cpin_q_task);
    AsyncTask_SetRoute(at_creg_q_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_creg_q_task);
    // AsyncTask_SetRoute(at_cgdcont_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_cgdcont_task);
    AsyncTask_SetRoute(at_cgact_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_cgact_task);
    AsyncTask_SetRoute(at_qicsgp_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_qicsgp_task);
    AsyncTask_SetRoute(at_qicfg_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_qicfg_task);
    AsyncTask_SetRoute(at_clk_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_clk_task);
    // AsyncTask_SetRoute(at_qicfg_close_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_qicfg_close_task);
    AsyncTask_SetRoute(at_qicfg_dataformat_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_qicfg_dataformat_task);

    AsyncTask_SetRoute(at_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_task);
    AsyncTask_SetRoute(at_cpin_q_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_cpin_q_task);
    AsyncTask_SetRoute(at_creg_q_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_creg_q_task);
    AsyncTask_SetRoute(at_cgdcont_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_cgdcont_task);
    AsyncTask_SetRoute(at_cgact_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_cgact_task);
    AsyncTask_SetRoute(at_qicsgp_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_qicsgp_task);
    AsyncTask_SetRoute(at_qicfg_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_qicfg_task);
    AsyncTask_SetRoute(at_clk_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_clk_task);
    // AsyncTask_SetRoute(at_qicfg_close_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_qicfg_close_task);
    AsyncTask_SetRoute(at_qicfg_dataformat_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_qicfg_dataformat_task);
}

void BFL_4G_TCP_TaskCreate(int sockid)
{
    at_qicsgp_task_send();
    // at_task_delay(5000);
    at_qicfg_dataformat_task1_send();
    at_qiopen_task_send(sockid);
    at_qisde_task_send(sockid);
    at_qiswtmd_task_send(sockid);
    at_task_delay(500);

    /*
    Invake Map:
    at_qiopen_task_send ->at_qisde_task_send->at_qiswtmd_task_send->at_task_delay
                        ->at_qiopen_task_send
                        at_qiswtmd_task_send执行完成才能写数据。
    */
}

void BFL_4G_TCP_List_TaskCreate(int sockid)
{
    AsyncTask_t *at_qiopen_task = AsyncTask_Create(at_qiopen_task_send_, (void *)sockid, 0);
    AsyncTask_t *at_qisde_task = AsyncTask_Create(at_qisde_task_send_, (void *)sockid, 0);
    AsyncTask_t *at_qiswtmd_task = AsyncTask_Create(at_qiswtmd_task_send_, (void *)sockid, 0);
    context.sockeOpenTasks[sockid] = at_qiopen_task;

    AsyncTaskList_t *task_list = context.task_list;
    AsyncTaskList_StaticAdd(task_list, at_qiopen_task);
    AsyncTaskList_StaticAdd(task_list, at_qisde_task);
    AsyncTaskList_StaticAdd(task_list, at_qiswtmd_task);

    AsyncTask_SetRoute(at_qiopen_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_qiopen_task);
    AsyncTask_SetRoute(at_qisde_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_qisde_task);
    AsyncTask_SetRoute(at_qiswtmd_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), at_qicfg_close_task);

    AsyncTask_SetRoute(at_qiopen_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_qiopen_task);
    AsyncTask_SetRoute(at_qisde_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_qisde_task);
    AsyncTask_SetRoute(at_qiswtmd_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), at_qicfg_close_task);
}

int32_t AsyncTaskFuncResultMap(at_resp_code code)
{
    return (int32_t)code;
}

void at_curr_callback(at_response_t *r)
{
    AsyncTask_SetFuncResult(context.task_list->pCurrentTask, AsyncTaskFuncResultMap(r->code));
    AsyncTask_SetState(context.task_list->pCurrentTask, ASYNC_TASK_STATE_FINISHED);
}

void BFL_4G_IOInterfaceInit()
{
    // 创建基本任务批处理
    context.at_obj = at_obj_create(&at_adapter);
    if (context.at_obj == NULL)
    {
        ULOG_ERROR("at_obj_create failed\r\n");
        return;
    }
    at_obj_set_curr_at_cb(context.at_obj, at_curr_callback);
}

void BFL_4G_AsyncTaskExecContext_Init()
{
    if (!context.base_cfg_ok)
    {
        memset(&context, 0, sizeof(context));
        context.task_list = AsyncTaskList_Create();
        if (context.task_list == NULL)
        {
            ULOG_ERROR("[4G] Async task list create failed.");
        }

        CHIP_EC800M_Init();
        context.base_cfg_ok = true;
        context.writeIsUsing = 1; // 这里是想判断有没有成功初始化并且连接到服务器了，后面会更换标志位
        context.baseCfgIsOk = false;

        BFL_4G_IOInterfaceInit();
        BFL_4G_List_BaseCfgTaskCreate();
    }
}

/**
 * @brief
 *
 * @param sockid
 * @param writeBuf
 * @param uLen
 * @return uint32_t 实际写入的字节数，如果写入失败则返回0。可以通过BFL_4G_TCP_Task_Writeable()来
 * 查看是否可以写数据。
 */
uint32_t BFL_4G_TCPWrite_Task(int sockid, uint8_t *writeBuf, uint32_t uLen)
{
    uint32_t ret = 0;
    if (BFL_4G_TCP_Task_Writeable())
    {

        size_t wLen = 0;

        /**
         AT+QISENDEX=0,"3132333435" //发送 16 进制字符串数据。
         SEND OK
          */
        size_t wDataHexLen = 0;
        size_t wATBufRemainLen = AT_BUF_LEN - 2; // 最后一个字节留给'\0'和命令的'"'
        size_t wATBufLen = 0;
        wDataHexLen = uLen * 2;
        wATBufLen = sprintf((char *)at_output_buf, "AT+QISENDEX=%d,\"", sockid);
        wATBufRemainLen -= wATBufLen;

        wLen = wDataHexLen > wATBufRemainLen ? wATBufRemainLen : wDataHexLen;
        wLen = wLen / 2; // 转换为字节
        // 转换为16进制
        for (size_t i = 0; i < wLen; i++)
        {
            sprintf((char *)at_output_buf + wATBufLen + i * 2, "%02X", writeBuf[i]);
        }
        wATBufLen += wLen * 2;
        wATBufLen += sprintf((char *)at_output_buf + wATBufLen, "\"");

        AsyncTaskList_t *task_list = context.task_list;
        AsyncTask_t *at_qisend_task = AsyncTask_Create(at_qisend_task_send_, (void *)sockid, 0);
        if (at_qisend_task == NULL)
        {
            ULOG_ERROR("[Async] AT qisend task create failed.");
            ret = 0;

            for (;;)
            {
            }
        }
        else
        {
            context.write_buf = at_output_buf;
            context.write_buf_len = wLen;
            context.writeIsUsing = 1;

            AsyncTaskList_DynamicPush(task_list, at_qisend_task);
            AsyncTask_SetRoute(at_qisend_task, AsyncTaskFuncResultMap(AT_RESP_TIMEOUT), context.sockeOpenTasks[sockid]);
            AsyncTask_SetRoute(at_qisend_task, AsyncTaskFuncResultMap(AT_RESP_ERROR), context.sockeOpenTasks[sockid]);

            ret = wLen;
        }
    }
    return ret;
}

bool BFL_4G_TCP_Task_Writeable()
{
    return context.writeIsUsing == 0;
}

int socket0_recv_handler(at_urc_info_t *info);
int socket1_recv_handler(at_urc_info_t *info);
int socket2_recv_handler(at_urc_info_t *info);
int socket3_recv_handler(at_urc_info_t *info);

const char *at_socket_ucr_stem(int sockid)
{
    switch (sockid)
    {
    case SOCKET0:
        return "\"recv\",0";
        break;
    case SOCKET1:
        return "\"recv\",1";
        break;
    case SOCKET2:
        return "\"recv\",2";
        break;
    default:
        return "\"recv\",0";
        break;
    }
}

int (*at_socket_ucr_handler(int sockid))(at_urc_info_t *info)
{
    switch (sockid)
    {
    case SOCKET0:
        return socket0_recv_handler;
        break;
    case SOCKET1:
        return socket1_recv_handler;
        break;
    case SOCKET2:
        return socket2_recv_handler;
        break;
    default:
        return NULL;
        break;
    }
}

/**
 * @brief 不要重复初始化同一个sockid
 *
 * @param sockid
 */
void BFL_4G_TCP_Task_UCRTable_Init(int sockid)
{
    context.urc_table[context.urc_table_size].prefix = "+QIURC: \"recv\"";
    context.urc_table[context.urc_table_size].stem = "\r\n";
    context.urc_table[context.urc_table_size].endmark = '\r'; // 实际上内部会替换为'\0'
    context.urc_table[context.urc_table_size].handler = at_socket_ucr_handler(sockid);
    context.urc_table_size++;
    at_obj_set_urc(context.at_obj, context.urc_table, context.urc_table_size);
}

void BFL_4G_StartCalibrateTimeOneTimes_Task()
{
    AsyncTask_t *at_clk_task = AsyncTask_Create(at_clk_task_send, NULL, 0);
    AsyncTaskList_t *task_list = context.task_list;
    AsyncTaskList_DynamicPush(task_list, at_clk_task);
}

int socket0_recv_handler(at_urc_info_t *info)
{

    // find "+QIURC: "
    char *pData = strstr(info->urcbuf, "+QIURC: \"recv\",0,");
    pData += strlen("+QIURC: \"recv\",0,");
    char *pLenStr = pData;

    // find "\r\n"
    pData = strstr(info->urcbuf, "\r\n");
    char *pLenStrEnd = pData;
    *pLenStrEnd = '\0';
    pData += 2;

    // calc len
    int len = atoi(pLenStr);

    // find last "\r\n"
    char *pLastData = strstr(pData, "\r");
    if (pLastData != NULL)
    {
        *pLastData = '\0';
    }

    ULOG_INFO("[SOCKET0] %s len = %u", pData, len);
    uint8_t ele = 0;
    // HEX str to byte:"5B5365"->"[Ser"
    for (int i = 0; i < len; i++)
    {
        sscanf(pData + i * 2, "%02hhx", &ele);
        cqueue_in(&context.socketRevQueues[SOCKET0], &ele, 1);
    }
    return 0;
}

int socket1_recv_handler(at_urc_info_t *info)
{

    return 0;
}

int socket2_recv_handler(at_urc_info_t *info)
{

    return 0;
}

int socket3_recv_handler(at_urc_info_t *info)
{

    return 0;
}