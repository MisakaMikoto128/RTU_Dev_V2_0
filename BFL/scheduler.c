/**
 * @file scheduler.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-08
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "scheduler.h"
#include "mtime.h"
#include "HDL_CPU_Time.h"

/**
 * @brief 获取CPU的毫秒滴答计数
 *
 * @return uint32_t
 */
uint32_t getCPUMsTick();

/**
 * @brief 获取CPU的滴答计数
 *
 * @return uint32_t
 */
uint32_t getCPUTick();

/**
 * @brief 获取CPU的滴答计数的时间，单位ms
 *
 * @return float
 */
float getCPUOneTickTime();

uint32_t getCPUMsTick()
{
#if HDL_CPU_TIME_OEN_TICK_TIME == 1000 // 1ms
    return HDL_CPU_Time_GetTick();
#else
    return (uint32_t)(HDL_TICK_TO_TIME(HDL_CPU_Time_GetTick()));
#endif
}

uint32_t getCPUTick()
{
    return HDL_CPU_Time_GetTick();
}

float getCPUOneTickTime()
{
    return HDL_CPU_TIME_OEN_TICK_TIME / 1000.0f;
}

uint32_t MsToTicks(uint32_t ms)
{
    return (uint32_t)(HDL_TIME_TO_TICK(ms));
}

static struct sc_list _gSchedulerList = {NULL, NULL};
static struct sc_list *gSchedulerList = &_gSchedulerList;
static float oen_tick_time            = 1; // ms

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
那么到scheduler_handler中处理相应的task时会马上就执行一次。
*/

void Functional_execute(Functional_t *functional)
{
    if (functional->fun != NULL) {
        functional->fun(functional->arg);
    }
}

/**
 * @brief Scheduler初始化。
 *
 */
void scheduler_init()
{
    sc_list_init(gSchedulerList);
    oen_tick_time = getCPUOneTickTime();
}

/**
 * @brief scheduler处理器。
 * 只有保证scheduler_handler执行频率大于scheduler计时器的计时分辨率的频率才能保证时间相对准确的定时任务调度。
 */
void scheduler_handler()
{
    struct sc_list *it    = NULL;
    SchedulerTask_t *task = NULL;

    sc_list_foreach(gSchedulerList, it)
    {
        task = sc_list_entry(it, SchedulerTask_t, next);
        if (getCPUTick() >= (task->register_tick + task->delay_before_first_exe)) {
            if (task->_exe_cnt < task->exe_times) {
                // 这里一定是>=，如果是 > ，那么在1 cpu tick间隔的时候时间上是2cpu tick执行一次。
                // 这里不允许period为0，不然就会失去调度作用。
                // 这里需要保证一定的实时性
                if ((getCPUTick() - task->last_exe_tick) >= task->period) {

                    Functional_execute(&task->fun);
                    task->_elapsed_tick_since_last_exe = getCPUTick() - task->last_exe_tick;
                    task->_exe_tick_error              = task->_elapsed_tick_since_last_exe - task->period;
                    if (task->_exe_tick_error > 0) {
                        task->last_exe_tick = getCPUTick();
                    } else {
                        task->last_exe_tick += task->period;
                    }
                    task->_exe_cnt++;
                    task->_exe_cnt = task->_exe_cnt == SCHEDULER_EXE_TIMES_INF ? 0 : task->_exe_cnt;
                }
            }
        }
    }
}

/**
 * @brief 将任务节点注册到scheduler中。
 * @note 这个函数会避免period为0的情况，如果period为0，那么period会变为1。
 * @param sche_node
 * @retval true 注册成功，false 注册失败。
 */
bool scheduler_register(SchedulerTask_t *task)
{
    bool ret = false;
    if (task != NULL) {
        task->register_tick = getCPUTick();
        task->period        = task->period == 0 ? 1 : task->period;
        sc_list_init(&task->next);
        sc_list_add_tail(gSchedulerList, &task->next);
        ret = true;
    }
    return ret;
}

/**
 * @brief 任务是否是注册的状态。
 *
 * @param sche_node
 * @retval true 任然是注册的，false 未注册。
 */
bool scheduler_is_task_registered(SchedulerTask_t *task)
{
    bool ret = false;
    if (task != NULL) {
        struct sc_list *it     = NULL;
        SchedulerTask_t *_task = NULL;
        struct sc_list *item, *tmp;

        sc_list_foreach_safe(gSchedulerList, tmp, it)
        {
            _task = sc_list_entry(it, SchedulerTask_t, next);
            if (_task == task) {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

/**
 * @brief 取消已经注册到scheduler中的任务。
 * @note 这个方法会清空已经执行的次数。
 * @param task
 * @return true 如果任务注册过，且无其他原因导致取消注册失败。
 * @return false 任务未注册过，或者其他原因取消注册成功。
 */
bool scheduler_unregister(SchedulerTask_t *task)
{
    bool ret = false;
    if (task != NULL) {
        struct sc_list *it     = NULL;
        SchedulerTask_t *_task = NULL;
        struct sc_list *item, *tmp;

        sc_list_foreach_safe(gSchedulerList, tmp, it)
        {
            _task = sc_list_entry(it, SchedulerTask_t, next);
            if (_task == task) {
                // 清空task
                task->_exe_cnt = 0;

                sc_list_del(gSchedulerList, &_task->next);
                ret = true;
                break;
            }
        }
    }
    return ret;
}

/**
 * @brief 重置任务执行次数。
 *
 * @param task
 */
void scheduler_reset_exe_cnt(SchedulerTask_t *task)
{
    if (task != NULL) {
        task->_exe_cnt = 0;
    }
}

/**
 * @brief 设置任务节点的频率。
 *
 * @param task
 * @param freq 1-1000Hz
 * @return None
 */
void scheduler_set_freq(SchedulerTask_t *task, int freq)
{
    if (task != NULL) {
        task->period = 1000.0f / freq / oen_tick_time;
    }
}

/**
 * @brief 获取指定时间对应的tick数量。
 *
 * @param ms 时间，单位ms.
 * @return uint32_t tick数。
 */
uint32_t scheduler_get_ms_ticks(uint32_t ms)
{
    return ms / oen_tick_time;
}

static PeriodREC_t period_last_exe_tick_table[MAX_PERIOD_ID + 1] = {0};

/**
 * @brief 查询是否到了需要的周期。这个函数中高速查询，如果判断周期到了，就会
 * 返回true，否则返回false,并且当周期到了之后会更新last_exe_tick，保证每周期只会判
 * 断结果为真一次。用于在主循环中方便的构建周期性执行的代码段。
 *
 * 内置一个Period_t的最后一次执行时间的时间戳表，period_id标识。
 * @param period_id 周期id，全局唯一。
 * @param period 周期,单位tick。
 * @return true 周期到了
 * @return false 周期未到。
 */
bool period_query(uint8_t period_id, PeriodREC_t period)
{
    bool ret = false;

    // 这里一定是>=，如果是 > ，那么在1 cpu tick间隔的时候时间上是2cpu tick执行一次。
    // 这里不允许period为0，不然就会失去调度作用。
    if (((PeriodREC_t)getCPUTick() - period_last_exe_tick_table[period_id]) >= period) {
        period_last_exe_tick_table[period_id] = getCPUTick();
        ret                                   = true;
    }
    return ret;
}

/**
 * @brief 同period_query_user，只是时间记录再一个uint32_t*指向的变量中。
 *
 * @param period_recorder 记录运行时间的变量的指针。
 * @param period 周期,单位tick。
 * @return true 周期到了
 * @return false 周期未到。
 */
bool period_query_user(PeriodREC_t *period_recorder, PeriodREC_t period)
{
    bool ret = false;
    // 这里一定是>=，如果是 > ，那么在1 cpu tick间隔的时候时间上是2cpu tick执行一次。
    // 这里不允许period为0，不然就会失去调度作用。
    if ((getCPUTick() - *period_recorder) >= period) {
        *period_recorder = getCPUTick();
        ret              = true;
    }
    return ret;
}

/**
 * @brief 从第一次被调用这个方法处理delay_rec对象开始，是否经过了delay时间。
 *
 * @param delay_recorder
 * @param delay
 * @return true 延时条件满足
 * @return false
 */
bool delay_one_times(DelayREC_t *delay_rec, uint32_t delay)
{
    if (delay_rec->isStarted == false) {
        delay_rec->start      = getCPUTick();
        delay_rec->isStarted  = true;
        delay_rec->isFinished = false;
    } else if (delay_rec->isFinished == false) {
        if ((getCPUTick() - delay_rec->start) >= delay) {
            delay_rec->isFinished = true;
        }
    }

    return delay_rec->isFinished;
}