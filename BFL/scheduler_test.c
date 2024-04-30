/**
 * @file scheduler_test.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-08
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#include "scheduler_test.h"
#include "test_lib.h"
#include "mtime.h"

/*
task1会执行十次fun1,1000ms执行一次
task2会执行1次fun2，1000ms执行一次，取消注册task1
task3会一直执行fun3,1ms一次，func3每执行1000次打印结果一次。

这里fun2与fun1执行的周期相同，fun1在fun2中被停止，那么fun1执行两次、一次还是0次取决于
其在内部task list中的位置。总的来说就是任务之间周期相近时，任务之间相互取消注册的操作会
导致无法预测的运行次数，这在例如至少要运行一次的情况下是危险的。

测试结果如下：
fun3的判断为if (i == 20)
[Scheduler Test]: fun3:exe times: 20
[Scheduler Test]: fun1:exe times: 1
[Scheduler Test]: fun2:exe times: 1

fun3的判断为if (i % 1000 == 0)
if (i % 1000 == 0)
[Scheduler Test]: fun3:exe times: 0
[Scheduler Test]: fun2:exe times: 1
[Scheduler Test]: fun3:exe times: 1000
[Scheduler Test]: fun3:exe times: 2000
[Scheduler Test]: fun3:exe times: 3000
*/
SchedulerTask_t *gTask = NULL;
void fun1()
{
    static int i = 0;
    i++;
    ULOG_INFO("[Scheduler Test]: %s:exe times: %d\r\n", __func__, i);
}

void fun2()
{
    static int i = 0;
    i++;

    ULOG_INFO("[Scheduler Test]: %s:exe times: %d\r\n", __func__, i);
    scheduler_unregister(gTask); // not recommend
}

void fun3()
{
    static int i = 0;
    if (i % 1000 == 0) {
        ULOG_INFO("[Scheduler Test]: %s:exe times: %d\r\n", __func__, i);
    }
    i++;
}

/**
 * @brief Scheduler调度器测试。
 *
 */
void scheduler_test()
{
    ulog_init_user();
    HDL_CPU_Time_Init();
    scheduler_init();

    SchedulerTask_t task1 =
        {
            .exe_times = 10,
            .period    = 1000,
            .fun       = fun1,
        };
    SchedulerTask_t task2 =
        {
            .exe_times = 1,
            .period    = 1000,
            .fun       = fun2,
        };
    gTask = &task1;
    SchedulerTask_t task3 =
        {
            .exe_times = SCHEDULER_EXE_TIMES_INF,
            .period    = 1,
            .fun       = fun3,
            ._exe_cnt  = SCHEDULER_EXE_TIMES_INF - 10,
        };
    scheduler_register(&task1);
    scheduler_register(&task2);
    scheduler_register(&task3);

    while (1) {
        scheduler_handler();
    }
}

/**
 * @brief Period方法测试。
 *
 */
void period_test()
{
    ulog_init_user();
    HDL_CPU_Time_Init();
    while (1) {
        if (period_query(0, 1000)) {
            fun1();
        }
    }
}