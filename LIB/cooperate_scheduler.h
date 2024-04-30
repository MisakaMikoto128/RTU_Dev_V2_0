/**
 * @file cooperate_scheduler.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-12-14
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#ifndef cooperate_scheduler_H
#define cooperate_scheduler_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limits.h>
#include "sc_list.h"
#include "HDL_G4_CPU_Time.h"

/* Config CPU Tick here */
#define scheduler_get_cpu_tick() HDL_G4_CPU_Time_GetTick()

typedef bool (*Function_t)(void *arg);

typedef struct tagFunctional_t
{
    Function_t fun;
    void *arg;
} Functional_t;

typedef struct tagFunctionalListNode_t
{
    char *name;
    /*上一次执行的CPU时间戳：每次执行的时候或者第一次启动任务的时候更新。
    当当前CPU tick >= last_exe_tick+exe_times时执行，之后更新last_exe_tick。
    */
    uint32_t last_exe_tick; // 上一次调度并且执行成功了的时刻或者调整后的上一次的执行时刻
    uint32_t exe_times;     // 需要执行多少次：
    uint32_t exe_cnt;       // 执行了多少次
    uint32_t exe_fail_cnt;  // 执行失败了多少次
    uint32_t period;        // 执行周期，单位为CPU tick的周期，必须大于0，否则就是相当于死循环了。

    uint32_t delay_before_first_exe;       // 从注册第一次真正执行调度的延迟
    uint32_t register_tick;                // 注册时刻
    Functional_t fun;                      // 执行的方法
    int32_t _exe_tick_error;               // 执行时刻误差
    uint32_t _elapsed_tick_since_last_exe; // 距离上传执行逝去了多少事件

    struct sc_list next;
} TaskNode_t;

typedef struct tagCooperativeGroup_t
{
    // 每个group中每个方法的最小执行时间间隔,如果一个组占用的资源总是呗占用的，那么会
    // 以这个min_period轮询当前任务，直到资源被释放，这个任务才会执行成功(获取到资源)。
    // 单位：tick,范围 >= 1。任务组中任务最小资源占用时间指的是
    // 任务组中的任务都会占用同一个全局资源，每个任务占用的时间可能不一样，这里设置
    // 一个最小的占用时间，用于任务组内任务请求资源失败的延时。设置为最小是为了保证
    // 调度频率的准确性。
    uint32_t min_period;
    struct sc_list task_list; // for TaskNode_t
    struct sc_list next;      // for self
} CooperativeGroup_t;

#define cooperate_scheduler_EXE_TIMES_INF UINT_MAX

typedef struct tagCooperateScheduler_t
{
    struct sc_list group_list; // for CooperativeGroup_t
} CooperateScheduler_t;

/**
 * @brief cooperate_scheduler初始化。
 *
 */
void cooperate_scheduler_init();

/**
 * @brief cooperate_scheduler处理器。
 * 只有保证cooperate_scheduler_handler执行频率大于cooperate_scheduler计时器的计时分辨率的频率才能保证时间相对准确的定时任务调度。
 */
void cooperate_scheduler_handler();

/**
 * @brief 将任务组点注册到cooperate_scheduler中。
 *
 * @param sche_node
 * @retval true 注册成功，false 注册失败。
 */
bool cooperate_scheduler_register(CooperativeGroup_t *group);

/**
 * @brief 任务是否是注册的状态。
 *
 * @param sche_node
 * @retval true 任然是注册的，false 未注册。
 */
bool cooperate_scheduler_is_group_registered(CooperativeGroup_t *group);

/**
 * @brief 取消已经注册到cooperate_scheduler中的任务组。这个方法不会改变
 * 任务组中曾经执行的记录。
 *
 * @param group
 * @return true 如果任务注册过，且无其他原因导致取消注册失败。
 * @return false 任务未注册过，或者其他原因取消注册成功。
 */
bool cooperate_scheduler_unregister(CooperativeGroup_t *group);

/**
 * @brief Scheduler初始化。
 *
 */
void cooperate_group_init(CooperativeGroup_t *group);

/**
 * @brief 设置任务组中任务最小资源占用时间。任务组中任务最小资源占用时间指的是
 * 任务组中的任务都会占用同一个全局资源，每个任务占用的时间可能不一样，这里设置
 * 一个最小的占用时间，用于任务组内任务请求资源失败的延时。设置为最小是为了保证
 * 调度频率的准确性。
 * 
 * @param group 
 * @param ms >1
 */
void cooperate_group_set_min_resoure_occupation_time(CooperativeGroup_t* group,uint32_t ms);

/**
 * @brief 将任务节点注册到协同组中。
 *
 * @param group 协同组对象指针。
 * @param task 任务指针。
 * @return true 注册成功。
 * @return false 注册失败。
 */
bool cooperate_group_register(CooperativeGroup_t *group, TaskNode_t *task);

/**
 * @brief 判断任务节点是否已经注册到协同组中。
 *
 * @param group 协同组对象指针。
 * @param task 任务指针。
 * @return true 在任务组中。
 * @return false 不在任务组中。
 */
bool cooperate_group_is_task_registered(CooperativeGroup_t *group, TaskNode_t *task);

/**
 * @brief 取消已经注册到任务组中的任务。这个方法不会改变
 * 任务组中曾经执行的记录。
 * @param task
 * @return true 如果任务注册过，且无其他原因导致取消注册失败。
 * @return false 任务未注册过，或者其他原因取消注册成功。
 */
bool cooperate_group_unregister(CooperativeGroup_t *group, TaskNode_t *task);

/**
 * @brief 设置任务节点的频率。
 *
 * @param task
 * @param freq 1-1000Hz
 * @return None
 */
void cooperate_scheduler_set_task_freq(TaskNode_t *task, int freq);

#endif //! cooperate_scheduler_H