#ifndef _IIC_H
#define _IIC_H
#include "stm32g4xx.h"

#define IIC_SCL_1    LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_15) /* SCL = 1 */ // PA15
#define IIC_SCL_0    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_15)             /* SCL = 0 */

#define IIC_SDA_1    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_7) /* SDA = 1 */ // PB7
#define IIC_SDA_0    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_7)             /* SDA = 0 */

#define IIC_READ_SDA LL_GPIO_IsInputPinSet(GPIOB, LL_GPIO_PIN_7) /* ?SDA输入 */

// IIC所有操作函数
void IIC_Init(void);                      // 初始化IIC的IO口
void IIC_Start(void);                     // 发送IIC开始信号
void IIC_Stop(void);                      // 发送IIC停止信号
void IIC_Send_Byte(uint8_t txd);          // IIC发送一个字节
uint8_t IIC_Read_Byte(unsigned char ack); // IIC读取一个字节
uint8_t IIC_Wait_Ack(void);               // IIC等待ACK信号
void IIC_Ack(void);                       // IIC发送ACK信号
void IIC_NAck(void);                      // IIC不发送ACK信号
#endif
