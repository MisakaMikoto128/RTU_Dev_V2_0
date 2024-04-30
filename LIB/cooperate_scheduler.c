/**
 * @file cooperate_scheduler.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-12-14
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "cooperate_scheduler.h"

static CooperateScheduler_t _gcooperate_scheduler;

static float oen_tick_time = 1; // ms

/*
这个调度器原理：
如果exe_cnt < exe_times
    那么每执行完成一次任务，exe_cnt++。
否则
    当当前tick - last_exe_tick > period执行任务，即时间到了就执行被任务
    不管任务是否成功都会开始下一次计时，也就是更新last_exe_tick

一般来说last_exe_tick在第一次注册后分两种情况：
    第一次初始化那么last_exe_tick为0，如果cpu已经运行超过一秒(或者tick溢出后超过1s)
    复用之前注册过的task，且距离上次取消注册超过一秒(或者tick溢出后超过1s)
那么到cooperate_scheduler_handler中处理相应的task时会马上就执行一次。
*/

/**
 * @brief 一个Group通常是一系列占用同一资源的方法，这个方法返回的true表示的是这次执行调度时发现资源任然忙碌，
 * 调度时考虑几点问题：
 *  1. 上次执行时刻到本次调度的时刻时间差大于执行周期
 *  2. 一个组内的task调度点几乎都在同一时刻，这样执行失败的次数会增加，怎样减少执行失败的次数
 *  3. 考虑执行频率是否符合预期，执行间隔是否足够均匀
 * @param functional
 * @return true
 * @return false
 */
bool Functional_execute(Functional_t *functional)
{
    bool exe_source_free = false;
    if (functional->fun != NULL)
    {
        exe_source_free = functional->fun(functional->arg);
    }
    return exe_source_free;
}

/**
 * @brief cooperate_scheduler初始化。
 *
 */
void cooperate_scheduler_init()
{
    sc_list_init(&_gcooperate_scheduler.group_list);
}

/**
 * @brief cooperate_scheduler处理器。
 * 只有保证cooperate_scheduler_handler执行频率大于cooperate_scheduler计时器的计时分辨率的频率才能保证时间相对准确的定时任务调度。
 */
void cooperate_scheduler_handler()
{
    static struct sc_list *it = NULL;
    static CooperativeGroup_t *group = NULL;
    static const struct sc_list *group_list = &_gcooperate_scheduler.group_list;

    static struct sc_list *it_task = NULL;
    static TaskNode_t *task = NULL;
    // Group所共用的资源是否是空闲的
    static bool exe_source_free = false;
    static struct sc_list *poped = NULL;
    sc_list_foreach(group_list, it)
    {
        group = sc_list_entry(it, CooperativeGroup_t, next);
        sc_list_foreach(&group->task_list, it_task)
        {
            task = sc_list_entry(it_task, TaskNode_t, next);
            if (scheduler_get_cpu_tick() >= (task->register_tick + task->delay_before_first_exe))
            {
                if (task->exe_cnt < task->exe_times)
                {
                    // 这里一定是>=，如果是 > ，那么在1 cpu tick间隔的时候时间上是2cpu tick执行一次。
                    // 这里不允许period为0，不然就会失去调度作用。
                    // 这里需要保证一定的实时性
                    if ((scheduler_get_cpu_tick() - task->last_exe_tick) >= task->period)
                    {
                        exe_source_free = Functional_execute(&task->fun);
                        if (exe_source_free)
                        {
                            task->_elapsed_tick_since_last_exe = scheduler_get_cpu_tick() - task->last_exe_tick;
                            task->_exe_tick_error = task->_elapsed_tick_since_last_exe - 2 * task->period;
                            if (task->_exe_tick_error > 0)
                            {
                                // 调整后的上一次的执行时刻
                                task->last_exe_tick = scheduler_get_cpu_tick();
                            }
                            else
                            {
                                task->last_exe_tick += task->period;
                            }
                            task->exe_cnt++;
                            task->exe_cnt = task->exe_cnt == cooperate_scheduler_EXE_TIMES_INF ? 0 : task->exe_cnt;

                            poped = sc_list_pop_head(&group->task_list);
                            sc_list_add_tail(&group->task_list, poped);
                        }
                        else
                        {
                            task->_elapsed_tick_since_last_exe = scheduler_get_cpu_tick() - task->last_exe_tick;
                            task->_exe_tick_error = task->_elapsed_tick_since_last_exe - 2 * task->period;
                            if (task->_exe_tick_error > 0)
                            {
                                // 调整后的上一次的执行时刻
                                task->last_exe_tick = scheduler_get_cpu_tick();
                            }
                            else
                            {
                                // 这个任务这次发现资源忙以后会等待1个tick再去执行Functional_execute看资源是否可用
                                task->last_exe_tick += group->min_period;
                            }
                            task->exe_fail_cnt++;
                        }
                    }
                }
            }
        }
    }
}

bool cooperate_scheduler_register(CooperativeGroup_t *group)
{
    bool ret = false;
    if (group != NULL)
    {
        group->min_period = 1;
        sc_list_init(&group->next);
        sc_list_add_tail(&_gcooperate_scheduler.group_list, &group->next);
        ret = true;
    }
    return ret;
}

bool cooperate_scheduler_is_group_registered(CooperativeGroup_t *group)
{
    bool ret = false;
    if (group != NULL)
    {
        struct sc_list *it = NULL;
        CooperativeGroup_t *_group = NULL;
        struct sc_list *item, *tmp;

        sc_list_foreach_safe(&_gcooperate_scheduler.group_list, tmp, it)
        {
            _group = sc_list_entry(it, CooperativeGroup_t, next);
            if (_group == group)
            {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

bool cooperate_scheduler_unregister(CooperativeGroup_t *group)
{
    bool ret = false;
    if (group != NULL)
    {
        struct sc_list *it = NULL;
        CooperativeGroup_t *_group = NULL;
        struct sc_list *item, *tmp;

        sc_list_foreach_safe(&_gcooperate_scheduler.group_list, tmp, it)
        {
            _group = sc_list_entry(it, CooperativeGroup_t, next);
            if (_group == group)
            {
                sc_list_del(&_gcooperate_scheduler.group_list, &_group->next);
                ret = true;
                break;
            }
        }
    }
    return ret;
}

void cooperate_group_init(CooperativeGroup_t *group)
{
    sc_list_init(&group->task_list);
}

/**
 * @brief 设置任务组中任务最小资源占用时间。任务组中任务最小资源占用时间指的是
 * 任务组中的任务都会占用同一个全局资源，每个任务占用的时间可能不一样，这里设置
 * 一个最小的占用时间，用于任务组内任务请求资源失败的延时。设置为最小是为了保证
 * 调度频率的准确性。
 * 
 * @param group 
 * @param ms > 1
 */
void cooperate_group_set_min_resoure_occupation_time(CooperativeGroup_t* group,uint32_t ms)
{
    group->min_period = ms / oen_tick_time;
    group->min_period = group->min_period == 0 ? 1 : group->min_period;
}

/**
 * @brief 将任务节点注册到协同组中。
 *
 * @param group 协同组对象指针。
 * @param task 任务指针。
 * @return true 注册成功。
 * @return false 注册失败。
 */
bool cooperate_group_register(CooperativeGroup_t *group, TaskNode_t *task)
{
    bool ret = false;
    if (task != NULL)
    {
        task->register_tick = scheduler_get_cpu_tick();
        task->period = task->period == 0 ? 1 : task->period;
        sc_list_init(&task->next);
        sc_list_add_tail(&group->task_list, &task->next);
        ret = true;
    }
    return ret;
}

/**
 * @brief 判断任务节点是否已经注册到协同组中。
 *
 * @param group 协同组对象指针。
 * @param task 任务指针。
 * @return true 在任务组中。
 * @return false 不在任务组中。
 */
bool cooperate_group_is_task_registered(CooperativeGroup_t *group, TaskNode_t *task)
{
    bool ret = false;
    if (task != NULL)
    {
        struct sc_list *it = NULL;
        TaskNode_t *_task = NULL;
        struct sc_list *item, *tmp;

        sc_list_foreach_safe(&group->task_list, tmp, it)
        {
            _task = sc_list_entry(it, TaskNode_t, next);
            if (_task == task)
            {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

/**
 * @brief 取消已经注册到任务组中的任务。
 *
 * @param task
 * @return true 如果任务注册过，且无其他原因导致取消注册失败。
 * @return false 任务未注册过，或者其他原因取消注册成功。
 */
bool cooperate_group_unregister(CooperativeGroup_t *group, TaskNode_t *task)
{
    bool ret = false;
    if (task != NULL)
    {
        struct sc_list *it = NULL;
        TaskNode_t *_task = NULL;
        struct sc_list *item, *tmp;

        sc_list_foreach_safe(&group->task_list, tmp, it)
        {
            _task = sc_list_entry(it, TaskNode_t, next);
            if (_task == task)
            {
                sc_list_del(&group->task_list, &_task->next);
                ret = true;
                break;
            }
        }
    }
    return ret;
}

/**
 * @brief 设置任务节点的频率。
 *
 * @param group
 * @param freq 1-1000Hz
 * @return None
 */
void cooperate_scheduler_set_task_freq(TaskNode_t *task, int freq)
{
    if (task != NULL)
    {
        task->period = 1000.0f / freq / oen_tick_time;
    }
}