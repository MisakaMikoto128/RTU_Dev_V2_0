#include "iic.h"
#include "main.h"
#include "HDL_G4_CPU_Time.h"

void IIC_Init()
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Pull = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
void SDA_IN()
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void SDA_OUT()
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT; //推挽输出
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  IIC_SDA_1;
}

//产生IIC起始信号
void IIC_Start(void)
{
  SDA_OUT();
  IIC_SDA_1;
  IIC_SCL_1;
  HDL_G4_CPU_Time_DelayUs(4);
  IIC_SDA_0;
  HDL_G4_CPU_Time_DelayUs(4);
  IIC_SCL_0;
}
//产生IIC停止信号
void IIC_Stop(void)
{
  SDA_OUT();
  IIC_SCL_0;
  IIC_SDA_0;
  HDL_G4_CPU_Time_DelayUs(4);
  IIC_SCL_1;
  IIC_SDA_1;
  HDL_G4_CPU_Time_DelayUs(4);
}
//等待应答信号到来
//返回值：1，接收应答失败
//        0，接收应答成功
uint8_t IIC_Wait_Ack(void)
{
  uint8_t errCount = 0;
  SDA_IN();
  IIC_SDA_1;
  HDL_G4_CPU_Time_DelayUs(2);
  IIC_SCL_1;
  HDL_G4_CPU_Time_DelayUs(2);
  while (IIC_READ_SDA)
  {
    errCount++;
    if (errCount > 250)
    {
      IIC_Stop();
      return 1;
    }
  }
  IIC_SCL_0;

  return 0;
}
//产生ACK应答
void IIC_Ack(void)
{
  IIC_SCL_0;
  SDA_OUT();
  IIC_SDA_0;
  HDL_G4_CPU_Time_DelayUs(2);
  IIC_SCL_1;
  HDL_G4_CPU_Time_DelayUs(2);
  IIC_SCL_0;
}
//不产生ACK应答
void IIC_NAck(void)
{
  IIC_SCL_0;
  SDA_OUT();
  IIC_SDA_1;
  HDL_G4_CPU_Time_DelayUs(2);
  IIC_SCL_1;
  HDL_G4_CPU_Time_DelayUs(2);
  IIC_SCL_0;
}
// IIC发送一个字节
//返回从机有无应答
// 1，有应答
// 0，无应答
void IIC_Send_Byte(uint8_t txd)
{
  uint8_t t;
  SDA_OUT();
  IIC_SCL_0;
  for (t = 0; t < 8; t++)
  {
    if ((txd & 0x80) >> 7)
      IIC_SDA_1;
    else
      IIC_SDA_0;
    txd <<= 1;
    HDL_G4_CPU_Time_DelayUs(2);
    IIC_SCL_1;
    HDL_G4_CPU_Time_DelayUs(2);
    IIC_SCL_0;
    HDL_G4_CPU_Time_DelayUs(2);
  }
}
//读1个字节，ack=1时，发送ACK，ack=0，发送nACK
uint8_t IIC_Read_Byte(unsigned char ack)
{
  unsigned char i, receive = 0;
  SDA_IN(); // SDA设置为输入
  for (i = 0; i < 8; i++)
  {
    IIC_SCL_0;
    HDL_G4_CPU_Time_DelayUs(2);
    IIC_SCL_1;
    receive <<= 1;
    if (IIC_READ_SDA)
      receive++;
    HDL_G4_CPU_Time_DelayUs(1);
  }
  if (!ack)
    IIC_NAck(); //发送nACK
  else
    IIC_Ack(); //发送ACK
  return receive;
}
