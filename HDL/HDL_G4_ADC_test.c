#include "HDL_ADC.h"
#include "HDL_ADC_test.h"
#include "log.h"

#include "HDL_CPU_Time.h"

void HDL_ADC_test() // 双重ADC数据采集，DMA传输
{
    HDL_CPU_Time_Init();
    ulog_init_user();

    uint32_t cpu_tick = HDL_CPU_Time_GetTick();

    LoopFrequencyTest_t loop_frq_test = {
        .measure_time = 1000, // 测试1秒钟
                              // 其他成员默认初始化为0.
    };

    HDL_ADC_Init();   // 初始化
    HDL_ADC_Enable(); // 使能

    while (1) {
        test_LoopFrequencyTest_handler(&loop_frq_test);

        if (test_LoopFrequencyTest_readable(&loop_frq_test)) {
            test_LoopFrequencyTest_show(&loop_frq_test, "Main");
            test_LoopFrequencyTest_reset(&loop_frq_test);
        }

        // 1s 执行一次
        if ((HDL_CPU_Time_GetTick() - cpu_tick) > 100) {
            cpu_tick = HDL_CPU_Time_GetTick();
            HDL_ADC_Vaule(&ConvertValue.ADC1_Value, &ConvertValue.ADC2_Value); // 计算数据，转化为十进制数，电压/mv
            Debug_Printf("[ADC Test]:ADC1_Value %d,ADC2_Value %d\r\n", ConvertValue.ADC1_Value, ConvertValue.ADC2_Value);
        }
    }
}
