/**
 * @file APP_Main.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-11-12
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 * @description 光伏RTU主程序。
 */
#include "APP_Main.h"
#include "APP_RTU_Sampler.h"
#include "BFL_RTU_Packet.h"
#include "CHIP_W25Q512_QueueFileSystem.h"
#include "scheduler.h"
#include "log.h"
#include "test.h"
#include <string.h>
#include "mytime.h"
#include "HDL_G4_Uart.h"
#include "HDL_G4_RTC.h"
#include "BFL_LED.h"
#include "HDL_G4_IWDG.h"
#include "HDL_G4_ADC.h"
#include <math.h>
#include <stdlib.h>

#include "APP_RTU_Sampler.h"
#include "BFL_RTU_Packet.h"
#include "BFL_4G.h"
#include "cqueue.h"
typedef struct tagSysInfo_t
{
  uint32_t devid;
  uint8_t mode;
} SysInfo_t;

typedef struct tagSysCtrl_t
{
  bool isTimeCalibrated;
  bool isSampling;
} SysCtrl_t;

typedef struct tagSys_t
{
  SysInfo_t info;
  SysCtrl_t ctrl;
} Sys_t;

Sys_t sys = {0};
/**
 * @brief 解析发送的RTU-远端数据包
 *
 * @param buf 数据包缓存。
 * @param output_type 输出类型，0原始字节码，1解析数据。
 */
void temp_humi_test(sc_byte_buffer *buf, int output_type);
void sensor_code_displacement_test(sc_byte_buffer *buf, int output_type);
void anchor_cable_gauge_test(sc_byte_buffer *buf, int output_type);
void wind_speed_and_direction_test(sc_byte_buffer *buf, int output_type);
void solar_radiation_test(sc_byte_buffer *buf, int output_type);
void attitude_sensor_test(sc_byte_buffer *buf, int output_type);
void inclination_angle_sensor_test(sc_byte_buffer *buf, int output_type);
void wind_speed_and_direction_test(sc_byte_buffer *buf, int output_type);

static uint8_t rtu_packet_buf[1024 * 2] = {0};
static uint8_t aBuf[1500];

static uint8_t read_buf[200];
static uint8_t _4G_rev_buf[200];

#define RS485_3_DIR_PIN GPIO_PIN_10
#define RS485_3_DIR_Port GPIOD
#define RS485_3_DIR_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD)
void releaseModbus1()
{
  LL_GPIO_SetOutputPin(RS485_3_DIR_Port, RS485_3_DIR_PIN);
}

#include <stdlib.h>
#include <math.h>
void setCalibrateTimeByUtcSecondsCb(uint64_t utcSeconds)
{
  uint64_t utcSecondsNow = HDL_G4_RTC_GetTimeTick(NULL);
  if (llabs((int64_t)(utcSeconds - utcSecondsNow)) > 1)
  {
    HDL_G4_RTC_SetTimeTick(utcSeconds);
    ULOG_INFO("[RTC] set time: %llu", utcSeconds);
  }
  else
  {
    ULOG_INFO("[RTC] time is ok : R %llu C %llu", utcSeconds, utcSecondsNow);
  }
  sys.ctrl.isTimeCalibrated = true;
}

#define RTU_DEV_ID_SET (0x1111111111111111ULL)
#define AppMainGetDevID() RTU_DEV_ID_SET

CQueue_t Sensor_Queue;
RTU_Sampling_Var_t Sensor_Queue_Buffer[60];
uint8_t Sensor_Queue_Init()
{
  return cqueue_create(&Sensor_Queue, Sensor_Queue_Buffer, sizeof(Sensor_Queue_Buffer) / sizeof(RTU_Sampling_Var_t), sizeof(RTU_Sampling_Var_t));
}

uint8_t Sensor_Queue_Write(RTU_Sampling_Var_t *var)
{
  return cqueue_enqueue(&Sensor_Queue, var);
}

uint8_t Sensor_Queue_Read(RTU_Sampling_Var_t *var)
{
  return cqueue_dequeue(&Sensor_Queue, var);
}
/**
 * @brief 光伏RTU主程序处理器。
 *
 */
void APP_Main()
{
  LoopFrequencyTest_t loop_frq_test = {
      .measure_time = 1000, // 测试1秒钟
                            // 其他成员默认初始化为0.
  };

  uint32_t total_write_amount_bak = 0;
  mytime_struct datetime;
  bool rest_task = false; // 当然重置任务标志位，当日已经执行为true,否则为false

  RTU_Sampling_Var_t rev_var;
  mytime_struct rev_datetime;
  RTU_Packet_t rev_packet;
  sc_byte_buffer _4G_rev_buffer;
  sc_byte_buffer_init(&_4G_rev_buffer, _4G_rev_buf, sizeof(_4G_rev_buf));

  HDL_G4_CPU_Time_Init();
  HDL_G4_RTC_Init();
  // HDL_G4_ADC_Init();
  // HDL_G4_ADC_Enable(); // 使能
  // HDL_G4_WATCHDOG_Init(20);
  ulog_init_user();

  Uart_Init(COM3, 115200, LL_LPUART_DATAWIDTH_8B, LL_LPUART_STOPBITS_1, LL_LPUART_PARITY_NONE);
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  RS485_3_DIR_Port_CLK_EN();
  releaseModbus1();
  /**/
  GPIO_InitStruct.Pin = RS485_3_DIR_PIN;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(RS485_3_DIR_Port, &GPIO_InitStruct);
  // 初始化时释放RS485总线
  releaseModbus1();

  BFL_4G_Init("IP", "CMIOT");
  BFL_4G_TCP_Init(SOCKET0, "8.135.10.183", 36335);
  BFL_4G_SetCalibrateTimeByUtcSecondsCb(setCalibrateTimeByUtcSecondsCb);

  RTU_Packet_t pkg = {0};
  BFL_RTU_Packet_init(&pkg, rtu_packet_buf, sizeof(rtu_packet_buf));
  int pushed_sample_var_num = 0;
  RTU_Sampling_Var_t vqr_header;
  RTU_Sampling_Var_t var;
  Sensor_Queue_Init();
  char buf[100];
  uint32_t r1 = 0;
  uint32_t r2 = 0;

  //	while (1)
  //  {
  //		uint32_t dwLen = Uart_Read(COM2, aBuf, 1000);
  //    if (dwLen > 0)
  //    {
  //			Uart_Write(COM1, aBuf, dwLen);
  //		}
  //		dwLen = Uart_Read(COM1, aBuf, 1000);
  //    if (dwLen > 0)
  //    {
  //			Uart_Write(COM2, aBuf, dwLen);
  //		}
  //	}

  uint32_t last_recv_time = 0;
 
  while (1)
  {
    uint32_t dwLen = Uart_Read(COM3, aBuf, 1000);
    if (dwLen > 0)
    {

      sc_byte_buffer_push_data(&_4G_rev_buffer, aBuf, dwLen);
 
      last_recv_time = HDL_G4_CPU_Time_GetTick();
      
      //      var.datetime = 1;
      //      var.ms = 1;
      //      // 放数据到采样点
      //      RTU_Sampling_Var_encoder_no_timestamp(&var, 1, &aBuf, dwLen);

      //      if (Sensor_Queue_Write(&var) == 0)
      //      {
      //        ULOG_ERROR("[ETP] sample point queue is full.");
      //      }
    }

    if((HDL_G4_CPU_Time_GetTick() - last_recv_time) > 2 && sc_byte_buffer_size(&_4G_rev_buffer) > 0)
    {
      BFL_4G_TCP_Write(SOCKET0, (uint8_t *)sc_byte_buffer_data_ptr(&_4G_rev_buffer), sc_byte_buffer_size(&_4G_rev_buffer));
    
      sc_byte_buffer_clear(&_4G_rev_buffer);
    }

    //    if (BFL_4G_TCP_Writeable(0) && Sensor_Queue_Read(&var))
    //    {
    //      // 存放数据到RTU数据包中(存放数据到RTU数据包的数据域)
    //      switch (pushed_sample_var_num)
    //      {
    //      case 0:
    //      {
    //        // 存放RTU设备代号
    //        uint64_t rtu_devid = AppMainGetDevID();
    //        // 编码采样点的时间和校验和
    //        RTU_Sampling_Var_encoder(&vqr_header, SENSOR_CODE_DEVICE_ID, &rtu_devid, sizeof(uint64_t));

    //        BFL_RTU_Packet_push_Sampling_Var(&pkg, &vqr_header);
    //        BFL_RTU_Packet_push_Sampling_Var(&pkg, &var);
    //        pushed_sample_var_num += 2;
    //      }
    //      break;
    //      case 22 - 1: // n - 1，包含中心标识采样点一个n个采样点
    //      {
    //        pushed_sample_var_num = 0;
    //        BFL_RTU_Packet_push_Sampling_Var(&pkg, &var);
    //        // 编码位字节流
    //        BFL_RTU_Packet_encoder(&pkg);
    //        // 发送字节流
    //        BFL_RTU_Packet_send_by_4G(&pkg, 1);
    //        // 每次使用完成后清空，防止上一次的数据带来的影响
    //        BFL_RTU_Packet_clear_buffer(&pkg);
    //      }
    //      break;
    //      default:
    //      {
    //        // 这里最好自己预先确定有多少数据需要存放到数据域
    //        BFL_RTU_Packet_push_Sampling_Var(&pkg, &var);
    //        pushed_sample_var_num++;
    //      }
    //      break;
    //      }
    //    }

    BFL_4G_Poll();

    //    if (BFL_4G_TCP_Readable(SOCKET0))
    //    {
    //      uint32_t len = BFL_4G_TCP_Read(SOCKET0, (uint8_t *)buf, 80);
    //      buf[len] = '\0';
    //      ULOG_INFO("Read:%s", buf);
    //    }
  }
}
