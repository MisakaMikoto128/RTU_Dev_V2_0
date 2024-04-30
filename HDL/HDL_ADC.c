#include "HDL_ADC.h"
#include "main.h"

/*内部双重ADC采集电压，定时器触发转换，DMA传输*/

/*需要用CUBEmx打开ADC，DMA，时钟，功能任意配置，以生成必要的头文件，输入时钟24Mhz，定时器外设总线时钟为170Mhz*/

/* 测试结果：测量的ADC值和万用表测量的值相比，ADC的值总是比万用表的值小了50mv左右，且电压在50mv以下时ADC测出数据总为0mv	原因：未知 */
/* 相同程序放在f4上可以正常测量0到3.3v。有可能是G4芯片本身的原因 */

ADC_Value ConvertValue; // 定义一个存放ADC数据的结构体变量

uint32_t ADC_ConvertValue[100]; // DMA传输数据的目标位置

/********************************************************
 *函数名：HDL_ADC_Init()
 *功能说明：配置需要的功能，GPIO、DMA、ADC、时钟、并初始化
 *形参：无
 *返回值：无
 *说明：函数放在CUBE生成的初始化函数后面，DMA初始化必须在ADC1初始化之前
 ********************************************************/
void HDL_ADC_Init()
{

    /*MX_GPIO_Init;*/
    /* GPIO通道时钟使能 */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOF); // F
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC); // C
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA); // A
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB); // B
    /*MX_GPIO_Init;*/

    /*MX_DMA_Init;*/
    /* DMA 时钟使能 */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMAMUX1);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    /* DMA 中断初始化 */
    /* DMA1_Channel1_IRQn 中断配置 */
    NVIC_SetPriority(DMA1_Channel1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    /*MX_DMA_Init;*/

    /*ADC1初始化;*/
    LL_ADC_InitTypeDef ADC_InitStruct             = {0};
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct     = {0};
    LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct           = {0};
    LL_RCC_SetADCClockSource(LL_RCC_ADC12_CLKSOURCE_SYSCLK);
    /* 外设时钟使能 */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC12);
    /*ADC1 GPIO 配置*/
    GPIO_InitStruct.Pin  = ADC1_IN1; // PA0引脚对应ADC1的通道1
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct); // GPIOx
    /* ADC1 DMA 初始化 */
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_ADC1);                        // DMA1通道1，ADC1请求
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY); // 数据传输方向,外设到内存
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_LOW);                // DMA优先级设置
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_CIRCULAR);                               // DMA1循环模式
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);                 // 传输的外设地址不增加
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);                   // 接收的内存地址增加
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_WORD);                       // 传输的外设数据宽度1字
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_WORD);                       // 接收的内存数据宽度1字
    /* Common config*/
    ADC_InitStruct.Resolution    = LL_ADC_RESOLUTION_12B;   // ADC分辨率12位
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT; // ADC采集数据右对齐
    ADC_InitStruct.LowPowerMode  = LL_ADC_LP_MODE_NONE;     // 关闭低功耗模式
    LL_ADC_Init(ADC1, &ADC_InitStruct);
    ADC_REG_InitStruct.TriggerSource    = LL_ADC_REG_TRIG_EXT_TIM_ADC_TRGO;  // ADC转换触发源为定时器事件触发
    ADC_REG_InitStruct.SequencerLength  = LL_ADC_REG_SEQ_SCAN_DISABLE;       // 关闭ADC扫描模式
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;    // 关闭ADC连续采样模式
    ADC_REG_InitStruct.ContinuousMode   = LL_ADC_REG_CONV_SINGLE;            // ADC单次采样模式
    ADC_REG_InitStruct.DMATransfer      = LL_ADC_REG_DMA_TRANSFER_UNLIMITED; // DMA连续传输
    ADC_REG_InitStruct.Overrun          = LL_ADC_REG_OVR_DATA_OVERWRITTEN;   // ADC寄存器数据溢出时采用覆盖写
    LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
    LL_ADC_SetGainCompensation(ADC1, 0);                                               // ADC无补偿
    LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);                             // 关闭过采集
    ADC_CommonInitStruct.CommonClock           = LL_ADC_CLOCK_SYNC_PCLK_DIV4;          // 时钟4分频
    ADC_CommonInitStruct.Multimode             = LL_ADC_MULTI_DUAL_REG_SIM_INJ_ALT;    // 两重ADC模式，规则同时和注入转换，ADC1数据存放在DR的低16，ADC2存放在DR的高16位
    ADC_CommonInitStruct.MultiDMATransfer      = LL_ADC_MULTI_REG_DMA_UNLMT_RES12_10B; // 单个DMA传输两个ADC的数据，ADC分辨率12bit，共32bit数据，DMA access mode
    ADC_CommonInitStruct.MultiTwoSamplingDelay = LL_ADC_MULTI_TWOSMP_DELAY_1CYCLE;     // 两个ADC采样的间隔为一个周期
    LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &ADC_CommonInitStruct);
    LL_ADC_REG_SetTriggerEdge(ADC1, LL_ADC_REG_TRIG_EXT_RISING); // ADC触发信号为上升沿
    /* 关闭低功耗模式 (enabled by default after reset state) */
    LL_ADC_DisableDeepPowerDown(ADC1);
    /* 启用ADC内部稳压调节 */
    LL_ADC_EnableInternalRegulator(ADC1);

    uint32_t wait_loop_index;
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while (wait_loop_index != 0) // 等待ADC稳定
    {
        wait_loop_index--;
    }
    /*配置ADC1规则通道*/
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, ADC1_CHANNEL);               // 1队，通道
    LL_ADC_SetChannelSamplingTime(ADC1, ADC1_CHANNEL, LL_ADC_SAMPLINGTIME_92CYCLES_5); // 通道的采样时间
    LL_ADC_SetChannelSingleDiff(ADC1, ADC1_CHANNEL, LL_ADC_SINGLE_ENDED);              // 选择单端口电压输入模式
    /*MX_ADC1_Init;*/

    /*MX_ADC2_Init;*/
    LL_RCC_SetADCClockSource(LL_RCC_ADC12_CLKSOURCE_SYSCLK);
    /* 外设时钟使能 */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC12);
    /*ADC2 GPIO 配置*/
    GPIO_InitStruct.Pin  = ADC2_IN1; // PA1引脚对应ADC2的通道2
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct); // GPIOx
    /* Common config*/
    ADC_InitStruct.Resolution    = LL_ADC_RESOLUTION_12B;
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.LowPowerMode  = LL_ADC_LP_MODE_NONE;
    LL_ADC_Init(ADC2, &ADC_InitStruct);
    ADC_REG_InitStruct.SequencerLength  = LL_ADC_REG_SEQ_SCAN_DISABLE;
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    ADC_REG_InitStruct.ContinuousMode   = LL_ADC_REG_CONV_SINGLE;
    ADC_REG_InitStruct.DMATransfer      = LL_ADC_REG_DMA_TRANSFER_NONE; // 无DMA
    ADC_REG_InitStruct.Overrun          = LL_ADC_REG_OVR_DATA_OVERWRITTEN;
    LL_ADC_REG_Init(ADC2, &ADC_REG_InitStruct);
    LL_ADC_SetGainCompensation(ADC2, 0);
    LL_ADC_SetOverSamplingScope(ADC2, LL_ADC_OVS_DISABLE);
    /* 关闭低功耗模式 (enabled by default after reset state) */
    LL_ADC_DisableDeepPowerDown(ADC2);
    /* 启用ADC内部稳压调节 */
    LL_ADC_EnableInternalRegulator(ADC2);

    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while (wait_loop_index != 0) {
        wait_loop_index--;
    }
    /* 配置ADC2规则通道 */
    LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_1, ADC2_CHANNEL); // 1队，通道
    LL_ADC_SetChannelSamplingTime(ADC2, ADC2_CHANNEL, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC2, ADC2_CHANNEL, LL_ADC_SINGLE_ENDED);
    /*MX_ADC2_Init;*/

    /*MX_TIM_Init;*/
    LL_TIM_InitTypeDef TIM_InitStruct = {0};
    /* 外设时钟使能 */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM_ADC); // 定时器时钟

    TIM_InitStruct.Prescaler     = TIM_Prescaler;         // 预分频
    TIM_InitStruct.CounterMode   = LL_TIM_COUNTERMODE_UP; // 向上计数
    TIM_InitStruct.Autoreload    = TIM_Autoreload;        // 计数值满后触发一次事件更新
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(TIM_ADC, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM_ADC);                           // 不使能预装载
    LL_TIM_SetClockSource(TIM_ADC, LL_TIM_CLOCKSOURCE_INTERNAL); // 内部时钟源
    LL_TIM_SetTriggerOutput(TIM_ADC, LL_TIM_TRGO_UPDATE);        // 定时器触发为事件更新
    LL_TIM_DisableMasterSlaveMode(TIM_ADC);                      // 不使能主从模式
    /*MX_TIM_Init;*/
}

/********************************************************
 *函数名：HDL_ADC_Enable()
 *功能说明：启动双重ADC，定时器触发转换，DMA运输
 *形参：无
 *返回值：无
 *说明：函数放在HDL_ADC_Init后面
 ********************************************************/
void HDL_ADC_Enable()
{
    LL_ADC_Enable(ADC1); // 使能ADC
    LL_ADC_Enable(ADC2); // 使能ADC

    /* 配置双重ADC，配置DMA传输参数，开启转换传输*/
    LL_TIM_EnableCounter(TIM_ADC);                                                                                                                                                 // 开启时钟
    LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_1, LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA_MULTI), (uint32_t)ADC_ConvertValue, LL_DMA_DIRECTION_PERIPH_TO_MEMORY); // DMA1通道1，传输地址，外设到内存，传输的数据放在ADC_ConvertValue
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, DMA_TransferCNT);                                                                                                                 // DMA传输数据的项目数
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);                                                                                                                                  // 使能DMA1通道1
    LL_ADC_REG_StartConversion(ADC1);                                                                                                                                              // 开启ADC组
}

/********************************************************
 *函数名：HDL_ADC_Vaule(uint16_t *adc1_Value,uint16_t *adc2_Value)
 *功能说明：计算ADC1的采集的数据和ADC2采集的数据
 *形参：uint16_t *adc1_Value存放ADC1的值，uint16_t *adc2_Value存放ADC2的值，单位mV
 *返回值：无
 *说明：函数放在循环中
 ********************************************************/
void HDL_ADC_Vaule(uint16_t *adc1_Value, uint16_t *adc2_Value)
{
    *adc1_Value = (ADC_ConvertValue[0] & 0xFFFF) * ADC_Vref / ADC_Resolution; // ADC1的数据放在ADC_ConvertValue低16位中，12位分辨率
    *adc2_Value = (ADC_ConvertValue[0] >> 16) * ADC_Vref / ADC_Resolution;    // ADC2的数据放在ADC_ConvertValue高16位中
}
