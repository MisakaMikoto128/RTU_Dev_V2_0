/**
 * @file HDL_CPU_Time.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-08
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "HDL_CPU_Time.h"
#include "main.h"

#define CPU_TIM    TIM1
#define CPU_US_TIM TIM2
/**
 * 定时器：TIM1
 * 中断优先级: 抢占0，子优先级0
 * 计数周期1ms
 * 主频：170MHz
 */

static __IO uint32_t uwCpuTick;
static CPU_Time_Callback_t _gCPUTickCallback = NULL;
bool cpu_time_init_flag                      = false;

/**
 * @brief CPU滴答时钟初始化。
 *
 */
void HDL_CPU_Time_Init()
{
    if (cpu_time_init_flag == true) {
        return;
    }

    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

    /* TIM1 interrupt Init */
    NVIC_SetPriority(TIM1_UP_TIM16_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);

    /* USER CODE BEGIN TIM1_Init 1 */

    /* USER CODE END TIM1_Init 1 */
    TIM_InitStruct.Prescaler = 17UL - 1; // CNM,SB LL libarary and stupid CUBEMX.
                                         // Work does not talk about money, research direction is not clear, graduation is difficult
    TIM_InitStruct.CounterMode       = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload        = 10000UL - 1;
    TIM_InitStruct.ClockDivision     = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM1, &TIM_InitStruct);

    LL_TIM_DisableARRPreload(TIM1);

    uint32_t tmpsmcr;

    /* Reset the SMS, TS, ECE, ETPS and ETRF bits */
    tmpsmcr = TIM1->SMCR;
    tmpsmcr &= ~(TIM_SMCR_SMS | TIM_SMCR_TS);
    tmpsmcr &= ~(TIM_SMCR_ETF | TIM_SMCR_ETPS | TIM_SMCR_ECE | TIM_SMCR_ETP);
    TIM1->SMCR = tmpsmcr;

    LL_TIM_SetClockSource(TIM1, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_RESET);
    LL_TIM_SetTriggerOutput2(TIM1, LL_TIM_TRGO2_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM1);

    LL_TIM_SetCounter(CPU_TIM, 0);
    LL_TIM_EnableIT_UPDATE(TIM1); // 更新中断使能
    LL_TIM_EnableCounter(TIM1);   // 计数使能

    // 微秒定时器初始化
    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    TIM_InitStruct.Prescaler         = 170UL - 1;
    TIM_InitStruct.CounterMode       = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload        = 0xFFFFFFFFUL;
    TIM_InitStruct.ClockDivision     = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM2, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM2);
    LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM2);

    LL_TIM_SetCounter(TIM2, 0);
    LL_TIM_DisableIT_UPDATE(TIM2);

    NVIC_SetPriority(TIM2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(TIM2_IRQn);

    LL_TIM_EnableCounter(TIM2); // 计数使能

    cpu_time_init_flag = true;
}

/**
 * @brief 获取CPU滴答时钟。
 *
 * @return uint32_t
 */
uint32_t HDL_CPU_Time_GetTick()
{
    return uwCpuTick;
}

/**
 * @brief 重置CPU滴答时钟。
 *
 */
void HDL_CPU_Time_ResetTick()
{
    uwCpuTick = 0;
}

/**
 * @brief This function is called to increment a global variable "uwTick"
 *        used as application time base.
 * @note In the default implementation, this variable is incremented each 1ms
 *       in SysTick ISR.
 * @note This function is declared as __weak to be overwritten in case of other
 *      implementations in user file.
 * @retval None
 */
static inline void HDL_CPU_IncTick(void)
{
    uwCpuTick++;
}

/**
 * @brief
 *
 * @return uint32_t
 */
uint32_t HDL_CPU_Time_GetUsTick()
{
    return LL_TIM_GetCounter(CPU_US_TIM);
}

/**
 * @brief 重置CPU微秒滴答时钟。
 *
 */
void HDL_CPU_Time_ResetUsTick()
{
    LL_TIM_SetCounter(CPU_US_TIM, 0);
}

/**
 * @brief This function provides minimum delay (in microsecond) based
 *        on variable incremented.
 * @note 这个函数使用了16bit定时器,需要注意的是Debug模式下即使停止在断点，这个
 * 定时器还是在运行。
 * @param DelayUs specifies the delay time length, in microsecond.
 * @retval None
 */

/**
 * @brief 微妙演示函数，使用硬件定时器的寄存器，需要注意的是Debug模式下即使停止在断点，这个
 * 定时器还是在运行。另外需要确定所使用的定时器计数寄存器的位宽，课参考@US_TIMER_BITWIDE，移
 * 植程序时需要修改这个宏定义。
 *
 * @param DelayUs
 */
void HDL_CPU_Time_DelayUs(UsTimer_t DelayUs)
{
    UsTimer_t tickstart = HDL_CPU_Time_GetUsTick();
    UsTimer_t wait      = DelayUs;

    while ((HDL_CPU_Time_GetUsTick() - tickstart) < wait) {
    }
}

/**
 * @brief This function provides minimum delay (in millisecond) based
 *       on variable incremented.
 *  @note 这个函数在中断中使用时必须保证调用这个函数的中断优先级低于CPU毫秒定时器中断的优先级。
 * @param DelayMs
 */
void HDL_CPU_Time_DelayMs(uint32_t DelayMs)
{
    uint32_t tickstart = HDL_CPU_Time_GetTick();
    uint32_t wait      = DelayMs;

    while ((HDL_CPU_Time_GetTick() - tickstart) < wait) {
    }
}

/* 保存 TIM定时中断到后执行的回调函数指针 */
static CPU_Time_Callback_t s_TIM_CallBack1;
static CPU_Time_Callback_t s_TIM_CallBack2;
static CPU_Time_Callback_t s_TIM_CallBack3;
static CPU_Time_Callback_t s_TIM_CallBack4;

/**
 * @brief 使用TIM2-5做单次定时器使用, 定时时间到后执行回调函数。可以同时启动4个定时器通道，互不干扰。
 *          定时精度正负1us （主要耗费在调用本函数的执行时间）
 *          TIM2和TIM5 是32位定时器。定时范围很大
 *          TIM3和TIM4 是16位定时器。
 *
 * @param _CC : 捕获比较通道几，1，2，3, 4
 * @param _uiTimeOut : 超时时间, 单位 1us. 对于16位定时器，最大 65.5ms; 对于32位定时器，最大 4294秒
 * @param _pCallBack : 定时时间到后，被执行的函数
 * @param _pArg : 定时时间到后，被执行的函数所需要参数的地址。
 * @retval None
 */
void HDL_CPU_Time_StartHardTimer(uint8_t _CC, UsTimer_t _uiTimeOut, void *_pCallBack)
{
    UsTimer_t cnt_now;
    UsTimer_t cnt_tar;
    TIM_TypeDef *TIMx = CPU_US_TIM;

    /* 无需补偿延迟，实测精度正负1us */
    cnt_now = LL_TIM_GetCounter(TIMx);
    cnt_tar = cnt_now + _uiTimeOut; /* 计算捕获的计数器值 */
    if (_CC == 1) {
        s_TIM_CallBack1 = (CPU_Time_Callback_t)_pCallBack;
        LL_TIM_OC_SetCompareCH1(TIMx, cnt_tar); /* 设置捕获比较计数器CC1 */
        LL_TIM_ClearFlag_CC1(TIMx);             /* 清除CC1中断标志 */
        LL_TIM_EnableIT_CC1(TIMx);              /* 使能CC1中断 */
    } else if (_CC == 2) {
        s_TIM_CallBack2 = (CPU_Time_Callback_t)_pCallBack;
        LL_TIM_OC_SetCompareCH2(TIMx, cnt_tar); /* 设置捕获比较计数器CC2 */
        LL_TIM_ClearFlag_CC2(TIMx);             /* 清除CC2中断标志 */
        LL_TIM_EnableIT_CC2(TIMx);              /* 使能CC2中断 */
    } else if (_CC == 3) {
        s_TIM_CallBack3 = (CPU_Time_Callback_t)_pCallBack;

        LL_TIM_OC_SetCompareCH3(TIMx, cnt_tar); /* 设置捕获比较计数器CC3 */
        LL_TIM_ClearFlag_CC3(TIMx);             /* 清除CC3中断标志 */
        LL_TIM_EnableIT_CC3(TIMx);              /* 使能CC3中断 */
    } else if (_CC == 4) {
        s_TIM_CallBack4 = (CPU_Time_Callback_t)_pCallBack;
        LL_TIM_OC_SetCompareCH4(TIMx, cnt_tar); /* 设置捕获比较计数器CC4 */
        LL_TIM_ClearFlag_CC4(TIMx);             /* 清除CC4中断标志 */
        LL_TIM_EnableIT_CC4(TIMx);              /* 使能CC4中断 */
    } else {
        return;
    }
}

/**
 * @brief 关闭硬件定时器。实际上就是关中断。
 * 如果定时器已经执行完，那么不会改变任何寄存器。
 * 如果定时器正在执行，且关闭的时刻不再临界条件，那么会关闭中断。
 * 如果刚好CCx中断标志置位但是还没有进入中断，且此时清楚了CCx中断标志,那么会进入中断
 * 但是却不能执行回调函数。
 *
 * 这个函数主要还是用于启动定时器后不需要了，要在定时中间关闭，不打算定时器去执行回调函数的情况。
 * @param _CC : 捕获比较通道几，1，2，3, 4
 */
void HDL_CPU_Time_StopHardTimer(uint8_t _CC)
{
    UsTimer_t cnt_now;
    UsTimer_t cnt_tar;
    TIM_TypeDef *TIMx = CPU_US_TIM;
    switch (_CC) {
        case 1: {
            LL_TIM_ClearFlag_CC1(TIMx); /* 清除CC1中断标志 */
            LL_TIM_DisableIT_CC1(TIMx); /* 禁能CC1中断 */
        } break;
        case 2: {
            LL_TIM_ClearFlag_CC2(TIMx); /* 清除CC2中断标志 */
            LL_TIM_DisableIT_CC2(TIMx); /* 禁能CC2中断 */
        } break;
        case 3: {
            LL_TIM_ClearFlag_CC3(TIMx); /* 清除CC3中断标志 */
            LL_TIM_DisableIT_CC3(TIMx); /* 禁能CC3中断 */
        } break;
        case 4: {
            LL_TIM_ClearFlag_CC4(TIMx); /* 清除CC4中断标志 */
            LL_TIM_DisableIT_CC4(TIMx); /* 禁能CC4中断 */
        } break;
    }
}

/**
 * @brief This function handles TIM1 update interrupt and TIM16 global interrupt.
 * @retval None
 */
void TIM1_UP_TIM16_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(CPU_TIM)) {
        HDL_CPU_IncTick();
        if (_gCPUTickCallback != NULL) {
            _gCPUTickCallback();
        }
        LL_TIM_ClearFlag_UPDATE(CPU_TIM);
    }
}

/**
 * @brief This function handles TIM2 global interrupt.
 * @retval None
 */
void TIM2_IRQHandler()
{
    TIM_TypeDef *TIMx = CPU_US_TIM;

    if (LL_TIM_IsEnabledIT_CC1(TIMx) && LL_TIM_IsActiveFlag_CC1(TIMx)) {
        LL_TIM_ClearFlag_CC1(TIMx); /* 清除CC1中断标志 */
        LL_TIM_DisableIT_CC1(TIMx); /* 禁能CC1中断 */

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM_CallBack1();
    }

    if (LL_TIM_IsEnabledIT_CC2(TIMx) && LL_TIM_IsActiveFlag_CC2(TIMx)) {
        LL_TIM_ClearFlag_CC2(TIMx); /* 清除CC2中断标志 */
        LL_TIM_DisableIT_CC2(TIMx); /* 禁能CC2中断 */

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM_CallBack2();
    }

    if (LL_TIM_IsEnabledIT_CC3(TIMx) && LL_TIM_IsActiveFlag_CC3(TIMx)) {
        LL_TIM_ClearFlag_CC3(TIMx); /* 清除CC3中断标志 */
        LL_TIM_DisableIT_CC3(TIMx); /* 禁能CC2中断 */

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM_CallBack3();
    }

    if (LL_TIM_IsEnabledIT_CC4(TIMx) && LL_TIM_IsActiveFlag_CC4(TIMx)) {
        LL_TIM_ClearFlag_CC4(TIMx); /* 清除CC4中断标志 */
        LL_TIM_DisableIT_CC4(TIMx); /* 禁能CC4中断 */

        /* 先关闭中断，再执行回调函数。因为回调函数可能需要重启定时器 */
        s_TIM_CallBack4();
    }
}

/**
 * @brief This function handles CPU_US_TIM global interrupt.设置CPU tick定时器的每次中断回调
 * 的函数。
 * @retval None
 */
void HDL_CPU_Time_SetCPUTickCallback(CPU_Time_Callback_t _pCallBack)
{
    _gCPUTickCallback = _pCallBack;
}