/**
 * @file AsyncTaskList.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2023-10-07
 * @last modified 2023-10-07
 *
 * @copyright Copyright (c) 2023 Liu Yuanlin Personal.
 *
 */
#include "AsyncTaskList.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief
 *
 * @param func 在其中任务真正执行完成后，需要：
 * 调用AsyncTask_SetState(pTask, ASYNC_TASK_STATE_FINISHED)。标志任务的实际完成。
 * 调用AsyncTask_SetFuncResult(pTask, result)。标志任务的执行结果。
 *
 * @param param
 * @param beforeReadyDelay
 * @return AsyncTask_t*
 */
AsyncTask_t *AsyncTask_Create(void *func, void *param, uint32_t beforeReadyDelay)
{
    AsyncTask_t *pTask = (AsyncTask_t *)malloc(sizeof(AsyncTask_t));
    if (pTask != NULL)
    {
        memset(pTask, 0, sizeof(AsyncTask_t));
        pTask->func = func;
        pTask->param = param;
        pTask->before_ready_delay = beforeReadyDelay;
        pTask->state = ASYNC_TASK_STATE_INITIAL;
        memset(pTask->route, NULL, sizeof(pTask->route));
        sc_list_init(&pTask->node);
    }
    return pTask;
}

void AsyncTask_Destroy(AsyncTask_t *pTask)
{
    if (pTask != NULL)
    {
        free(pTask);
    }
}

/**
 * @brief 执行处于运行态的任务。
 *
 * @param pTask
 */
void AsyncTask_Exec(AsyncTask_t *pTask)
{
    if (pTask != NULL)
    {
        if (pTask->func != NULL)
        {
            pTask->func(pTask->param);
        }
    }
}

/**
 * @brief
 *
 * @param pTask 如果为NULL，返回ASYNC_TASK_STATE_FINISHED。
 * @return enum ASYNC_TASK_STATE
 */
enum ASYNC_TASK_STATE AsyncTask_GetState(AsyncTask_t *pTask)
{
    if (pTask == NULL)
    {
        return ASYNC_TASK_STATE_FINISHED;
    }
    return pTask->state;
}

/**
 * @brief 没有设置的路径默认为空。
 *
 * @param pTask
 * @param funcResult
 * @param pNextTask
 */
void AsyncTask_SetRoute(AsyncTask_t *pTask, uint32_t funcResult, AsyncTask_t *pNextTask)
{
    if (pTask != NULL && funcResult < ASYNCTASK_NEXT_ROUTE_NUM)
    {
        pTask->route[funcResult] = pNextTask;
    }
}

AsyncTask_t *AsyncTask_GetRoute(AsyncTask_t *pTask, uint32_t funcResult)
{
    if (pTask != NULL && funcResult < ASYNCTASK_NEXT_ROUTE_NUM)
    {
        return pTask->route[funcResult];
    }
    return NULL;
}

void AsyncTask_SetState(AsyncTask_t *pTask, enum ASYNC_TASK_STATE state)
{
    if (pTask != NULL)
    {
        pTask->state = state;
    }
}

void AsyncTask_SetFuncResult(AsyncTask_t *pTask, int32_t result)
{
    if (pTask != NULL)
    {
        pTask->func_result = result;
    }
}

AsyncTaskList_t *AsyncTaskList_Create()
{
    AsyncTaskList_t *pList = (AsyncTaskList_t *)malloc(sizeof(AsyncTaskList_t));
    if (pList != NULL)
    {
        sc_list_init(&pList->staticList);
        sc_list_init(&pList->dynamicList);
        pList->state = ASYNC_TASK_LIST_STATE_INITIAL;
        pList->pCurrentTask = NULL;
    }
    return pList;
}
void AsyncTaskList_Destroy(AsyncTaskList_t *pList)
{
    if (pList != NULL)
    {
        AsyncTaskList_Clear(pList);
        free(pList);
    }
}

void AsyncTaskList_StaticAdd(AsyncTaskList_t *pList, AsyncTask_t *pTask)
{
    if (pList != NULL && pTask != NULL)
    {
        sc_list_add_tail(&pList->staticList, &pTask->node);
    }
}

void AsyncTaskList_StaticRemove(AsyncTaskList_t *pList, AsyncTask_t *pTask)
{
    if (pList != NULL && pTask != NULL)
    {
        sc_list_del(&pList->staticList, &pTask->node);
    }
}

AsyncTask_t *AsyncTaskList_Next(AsyncTaskList_t *pList)
{
    AsyncTask_t *res = NULL;
    AsyncTask_t *route = NULL;
    // 到这里pList->pCurrentTask是已经完成的了
    if (pList != NULL)
    {
        switch (pList->state)
        {
        case ASYNC_TASK_LIST_STATE_INITIAL:
            if (!sc_list_is_empty(&pList->staticList))
            {
                struct sc_list *iter;
                iter = pList->staticList.next;
                res = sc_list_entry(iter, AsyncTask_t, node);
                pList->state = ASYNC_TASK_LIST_STATE_RUN_IN_STATIC_LIST;
            }
            else if (!sc_list_is_empty(&pList->dynamicList))
            {
                res = AsyncTaskList_DynamicPop(pList);
                pList->state = ASYNC_TASK_LIST_STATE_RUN_IN_DYNAMIC_LIST;
            }
            else
            {
                // PASS
            }
            break;
        case ASYNC_TASK_LIST_STATE_RUN_IN_STATIC_LIST:
        {
            // 路由选择
            route = AsyncTask_GetRoute(pList->pCurrentTask, pList->pCurrentTask->func_result);
            if (route != NULL)
            {
                res = route;
            }
            else
            {
                if (AsyncTaskList_IsStaticLastTask(pList, pList->pCurrentTask))
                {
                    res = NULL;
                    pList->state = ASYNC_TASK_LIST_STATE_RUN_IN_DYNAMIC_LIST;
                }
                else
                {
                    struct sc_list *iter;
                    iter = pList->pCurrentTask->node.next;
                    res = sc_list_entry(iter, AsyncTask_t, node);
                }
            }
        }
        break;
        case ASYNC_TASK_LIST_STATE_RUN_IN_DYNAMIC_LIST:
        {
            // 路由选择
            route = AsyncTask_GetRoute(pList->pCurrentTask, pList->pCurrentTask->func_result);
            if (route != NULL)
            {
                pList->state = ASYNC_TASK_LIST_STATE_RUN_IN_STATIC_LIST;
                res = route;
            }
            else
            {
                res = AsyncTaskList_DynamicPop(pList);
            }

            AsyncTask_Destroy(pList->pCurrentTask);
            pList->pCurrentTask = NULL;
        }
        break;
        default:
            break;
        }

        // 重置任务
        if (res != NULL)
        {
            res->state = ASYNC_TASK_STATE_INITIAL;
            res->func_result = 0;
        }

        // 更新当前任务
        pList->pCurrentTask = res;
    }
    return res;
}

void AsyncTaskList_DynamicPush(AsyncTaskList_t *pList, AsyncTask_t *pTask)
{
    if (pList != NULL && pTask != NULL)
    {
        sc_list_add_tail(&pList->dynamicList, &pTask->node);
    }
}

/**
 * @brief
 *
 * @param pList
 * @return AsyncTask_t* 如果队列为空，返回NULL。
 */
AsyncTask_t *AsyncTaskList_DynamicPop(AsyncTaskList_t *pList)
{
    if (pList != NULL)
    {
        struct sc_list *iter = sc_list_pop_head(&pList->dynamicList);
        AsyncTask_t *pTask = sc_list_entry(iter, AsyncTask_t, node);
        return pTask;
    }
    return NULL;
}

void AsyncTaskList_Clear(AsyncTaskList_t *pList)
{
    if (pList != NULL)
    {
        sc_list_clear(&pList->staticList);
        sc_list_clear(&pList->dynamicList);
    }
}

/**
 * @brief 对于初始和终止时当前任务为空处理为：
 * AsyncTask_GetState返回ASYNC_TASK_STATE_FINISHED。
 * 并且通过AsyncTaskList_Next获取下一个任务。
 * 路由路径一点只能路由到静态任务表。
 * @param pList
 */

void AsyncTaskList_Exec(AsyncTaskList_t *pList)
{
    if (pList != NULL)
    {
        switch (AsyncTask_GetState(pList->pCurrentTask))
        {
        case ASYNC_TASK_STATE_INITIAL:
            pList->pCurrentTask->state = ASYNC_TASK_STATE_READY;
            pList->pCurrentTask->_ready_moment = AsyncTaskGetMsTick();
            break;
        case ASYNC_TASK_STATE_READY:
            if (AsyncTaskGetMsTick() - pList->pCurrentTask->_ready_moment >=
                pList->pCurrentTask->before_ready_delay)
            {
                AsyncTask_Exec(pList->pCurrentTask);
                pList->pCurrentTask->state = ASYNC_TASK_STATE_RUNNING;
            }
            break;
        case ASYNC_TASK_STATE_RUNNING:
            break;
        case ASYNC_TASK_STATE_FINISHED:
            AsyncTaskList_Next(pList);
            break;
        default:
            break;
        }
    }
}

bool AsyncTaskList_IsEmpty(AsyncTaskList_t *pList)
{
    if (pList != NULL)
    {
        return sc_list_is_empty(&pList->staticList) && sc_list_is_empty(&pList->dynamicList);
    }
    return true;
}

bool AsyncTaskList_IsStaticFirstTask(AsyncTaskList_t *pList, AsyncTask_t *pTask)
{
    if (pList != NULL && pTask != NULL)
    {
        return &pList->staticList == &pTask->node;
    }
    return false;
}

bool AsyncTaskList_IsStaticLastTask(AsyncTaskList_t *pList, AsyncTask_t *pTask)
{
    if (pList != NULL && pTask != NULL)
    {
        return pList->staticList.prev == &pTask->node;
    }
    return false;
}