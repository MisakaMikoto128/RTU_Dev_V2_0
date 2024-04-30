#include "asyn_sys.h"
#include "HDL_CPU_Time.h"
#include <stddef.h>
#define ASYN_SYS_MAX_FUNCS 11
static AsynSys_Functional_t _gAsynSysFuncs[ASYN_SYS_MAX_FUNCS] = {0};
/**
 * @brief 注册异步系统函数。如果函数已经存在，则不做任何操作。
 *
 * @param func
 */
void asyn_sys_register(AsynSys_Func_t func)
{
    for (size_t i = 0; i < ASYN_SYS_MAX_FUNCS; i++) {
        if (_gAsynSysFuncs[i].func == func) {
            return;
        }
    }
    for (size_t i = 0; i < ASYN_SYS_MAX_FUNCS; i++) {
        if (_gAsynSysFuncs[i].func == NULL) {
            _gAsynSysFuncs[i].func = func;
            return;
        }
    }
}
/**
 * @brief 注销异步系统函数。如果函数不存在，则不做任何操作。
 *
 * @param func
 */
void asyn_sys_unregister(AsynSys_Func_t func)
{
    for (size_t i = 0; i < ASYN_SYS_MAX_FUNCS; i++) {
        if (_gAsynSysFuncs[i].func == func) {
            _gAsynSysFuncs[i].func = NULL;
            return;
        }
    }
}

/**
 * @brief 轮询异步系统函数。
 *
 */
void asyn_sys_poll()
{
    for (size_t i = 0; i < ASYN_SYS_MAX_FUNCS; i++) {
        if (_gAsynSysFuncs[i].func != NULL) {
            _gAsynSysFuncs[i].func();
        }
    }
}

/**
 * @brief
 * @warning 该函数会阻塞当前任务，直到ms毫秒后才返回。不能在被注册为异步任务的函数中调用，
 * 否则会出现递归调用，导致栈溢出。
 *
 * @param ms
 */
void asyn_sys_delay(uint32_t ms)
{
    uint32_t start_tick = HDL_CPU_Time_GetTick();
    while (HDL_CPU_Time_GetTick() - start_tick < ms) {
        asyn_sys_poll();
    }
}
