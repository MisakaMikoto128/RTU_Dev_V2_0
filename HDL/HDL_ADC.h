#ifndef __HDL_ADC__
#define __HDL_ADC__

#include "stm32g4xx.h"
#include "math.h"
/*需要用CUBEmx打开ADC，DMA，时钟，功能任意配置，以生成必要的头文件，此工程的输入时钟24Mhz，定时器外设总线时钟为170Mhz*/

/*ADC引脚和通道定义*/
#define ADC1_IN1     LL_GPIO_PIN_0    // ADC1引脚
#define ADC2_IN1     LL_GPIO_PIN_1    // ADC2引脚
#define ADC1_CHANNEL LL_ADC_CHANNEL_1 // ADC1规则通道
#define ADC2_CHANNEL LL_ADC_CHANNEL_2 // ADC2规则通道

/*ADC1引脚与对应的通道
PA0   ------> ADC1_IN1	LL_ADC_CHANNEL_1
PA1   ------> ADC1_IN2	LL_ADC_CHANNEL_2
PA2   ------> ADC1_IN3	LL_ADC_CHANNEL_3
PA3   ------> ADC1_IN4	LL_ADC_CHANNEL_4
PB14  ------> ADC1_IN5	LL_ADC_CHANNEL_5
PC0   ------> ADC1_IN6	LL_ADC_CHANNEL_6
PC1   ------> ADC1_IN7	LL_ADC_CHANNEL_7
PC2   ------> ADC1_IN8	LL_ADC_CHANNEL_8
*/

/*ADC2引脚与对应的通道
PA0   ------> ADC2_IN1	LL_ADC_CHANNEL_1
PA1   ------> ADC2_IN2	LL_ADC_CHANNEL_2
PA6   ------> ADC2_IN3	LL_ADC_CHANNEL_3
PA7   ------> ADC2_IN4	LL_ADC_CHANNEL_4
PC4   ------> ADC2_IN5	LL_ADC_CHANNEL_5
PC0   ------> ADC2_IN6	LL_ADC_CHANNEL_6
PC1   ------> ADC2_IN7	LL_ADC_CHANNEL_7
PC2   ------> ADC2_IN8	LL_ADC_CHANNEL_8
*/

/*定时器定义*/
#define TIM_Prescaler                    169                           // 定时器预分频
#define TIM_Autoreload                   4999                          // 定时器自动重装载值
#define TIM_ADC                          TIM3                          // 定时器选择TIM3
#define LL_APB1_GRP1_PERIPH_TIM_ADC      LL_APB1_GRP1_PERIPH_TIM3      // APB外设
#define LL_ADC_REG_TRIG_EXT_TIM_ADC_TRGO LL_ADC_REG_TRIG_EXT_TIM3_TRGO // ADC数据转换触发源为TIM3

#define DMA_TransferCNT                  1          // DMA传输的项目个数
#define ADC_Resolution                   pow(2, 12) // 分辨率，2的12次方
#define ADC_Vref                         3300       // 参考电压/mv
typedef struct adc {
    uint16_t ADC1_Value; // ADC1电压
    uint16_t ADC2_Value; // ADC2电压
} ADC_Value;

extern ADC_Value ConvertValue; // 引用结构体变量
/* 在while(1)循环中
HDL_ADC_Vaule(&ConvertValue.ADC1_Value,&ConvertValue.ADC2_Value);
*/

void HDL_ADC_Init(void); // 配置需要的功能并初始化,函数放在MX_初始化函数后面，其中MX_DMA_Init必须在MX_ADC1_Init之前

void HDL_ADC_Enable(void); // 启动双重ADC，定时器触发转换，DMA运输,函数放在HDL_ADC_Init后面

void HDL_ADC_Vaule(uint16_t *adc1_Value, uint16_t *adc2_Value); // 计算ADC1的采集的数据和ADC2采集的数据，函数放在循环中

#endif
