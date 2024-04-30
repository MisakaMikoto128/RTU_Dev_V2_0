/**
 * @file AsyncTaskList.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-10-07
 * @last modified 2023-10-07
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#ifndef ASYNCTASKLIST_H
#define ASYNCTASKLIST_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include "sc_list.h"
#include "HDL_G4_CPU_Time.h"
#define AsyncTaskGetMsTick() HDL_G4_CPU_Time_GetTick()

#define ASYNCTASKLIST_VERSION "V1.0.0"

    typedef void (*AsyncTaskFunc_t)(void *pArg);

    /*
    任务状态:

    初始态
    就绪态
    运行态
    完成态
    初始态

    */

    enum ASYNC_TASK_STATE
    {
        ASYNC_TASK_STATE_INITIAL,
        ASYNC_TASK_STATE_READY,
        ASYNC_TASK_STATE_RUNNING,
        ASYNC_TASK_STATE_FINISHED,
    };

// 任务下一步切换时可选的路径数目
#define ASYNCTASK_NEXT_ROUTE_NUM 3


    enum ASYNC_TASK_LIST_STATE
    {
        ASYNC_TASK_LIST_STATE_INITIAL,
        ASYNC_TASK_LIST_STATE_RUN_IN_STATIC_LIST,
        ASYNC_TASK_LIST_STATE_RUN_IN_DYNAMIC_LIST,
    };

    typedef struct tagAsyncTask_t
    {
        struct sc_list node;

        // 任务进入就绪状态后，延时多长时间执行
        uint32_t before_ready_delay;
        uint32_t _ready_moment;

        enum ASYNC_TASK_STATE state;
        AsyncTaskFunc_t func;
        void *param;
        struct tagAsyncTask_t *route[ASYNCTASK_NEXT_ROUTE_NUM];
        int32_t func_result;
    } AsyncTask_t;

    typedef struct tagAsyncTaskList_t
    {
        struct sc_list staticList;
        struct sc_list dynamicList;
        AsyncTask_t *pCurrentTask;
        enum ASYNC_TASK_LIST_STATE state;
    } AsyncTaskList_t;

    AsyncTask_t *AsyncTask_Create(void* func, void *param, uint32_t beforeReadyDelay);
    void AsyncTask_Destroy(AsyncTask_t *pTask);
    void AsyncTask_Exec(AsyncTask_t *pTask);
    enum ASYNC_TASK_STATE AsyncTask_GetState(AsyncTask_t *pTask);
    void AsyncTask_SetRoute(AsyncTask_t *pTask, uint32_t funcResult, AsyncTask_t *pNextTask);
    AsyncTask_t *AsyncTask_GetRoute(AsyncTask_t *pTask, uint32_t funcResult);
    void AsyncTask_SetState(AsyncTask_t *pTask, enum ASYNC_TASK_STATE state);
    void AsyncTask_SetFuncResult(AsyncTask_t *pTask, int32_t result);

    AsyncTaskList_t *AsyncTaskList_Create();
    void AsyncTaskList_Destroy(AsyncTaskList_t *pList);
    void AsyncTaskList_StaticAdd(AsyncTaskList_t *pList, AsyncTask_t *pTask);
    void AsyncTaskList_StaticRemove(AsyncTaskList_t *pList, AsyncTask_t *pTask);
    AsyncTask_t *AsyncTaskList_Next(AsyncTaskList_t *pList);
    void AsyncTaskList_DynamicPush(AsyncTaskList_t *pList, AsyncTask_t *pTask);
    AsyncTask_t *AsyncTaskList_DynamicPop(AsyncTaskList_t *pList);
    void AsyncTaskList_Clear(AsyncTaskList_t *pList);
    void AsyncTaskList_Exec(AsyncTaskList_t *pList);
    bool AsyncTaskList_IsEmpty(AsyncTaskList_t *pList);
    bool AsyncTaskList_IsStaticFirstTask(AsyncTaskList_t *pList, AsyncTask_t *pTask);
    bool AsyncTaskList_IsStaticLastTask(AsyncTaskList_t *pList, AsyncTask_t *pTask);

#ifdef __cplusplus
}
#endif
#endif //! ASYNCTASKLIST_H
