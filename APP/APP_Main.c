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

#include <string.h>
#include "datetime.h"
#include "HDL_Uart.h"
#include "HDL_RTC.h"
#include "BFL_LED.h"
#include "HDL_IWDG.h"
#include "HDL_ADC.h"
#include "HDL_SPI.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "APP_RTU_Sampler.h"
#include "BFL_RTU_Packet.h"
#include "BFL_4G.h"
#include "cqueue.h"
#include "HDL_CPU_Time.h"
#include "test.h"

#include "./sdcard/bsp_spi_sdcard.h"
#include "./sdcard/sdcard_test.h"
#include "app_fatfs.h"

#include "usbd_cdc_if.h"
#include "usb_device.h"

typedef struct tagSysInfo_t {
    uint32_t devid;
    uint8_t mode;
} SysInfo_t;

typedef struct tagSysCtrl_t {
    bool isTimeCalibrated;
    bool isSampling;
} SysCtrl_t;

typedef struct tagSys_t {
    SysInfo_t info;
    SysCtrl_t ctrl;
} Sys_t;

Sys_t sys = {0};

static uint8_t rtu_packet_buf[1024 * 2] = {0};
static uint8_t aBuf[1500];
static uint8_t _4G_rev_buf[1500];
static uint8_t RxBuf[1200];

#define RS485_3_DIR_PIN           GPIO_PIN_10
#define RS485_3_DIR_Port          GPIOD
#define RS485_3_DIR_Port_CLK_EN() LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOD)
void releaseModbus1()
{
    LL_GPIO_SetOutputPin(RS485_3_DIR_Port, RS485_3_DIR_PIN);
}

void takeModbus1()
{
    LL_GPIO_ResetOutputPin(RS485_3_DIR_Port, RS485_3_DIR_PIN);
}

#include <stdlib.h>
#include <math.h>
void setCalibrateTimeByUtcSecondsCb(uint64_t unixSeconds)
{
    uint64_t utcSecondsNow = datetime_get_unix_timestamp();
    if (llabs((int64_t)(unixSeconds - utcSecondsNow)) > 1) {
        datetime_set_unix_timestamp(unixSeconds);
        ULOG_INFO("[RTC] set time: %llu", unixSeconds);
    } else {
        HDL_RTC_SetSynced();
        ULOG_INFO("[RTC] time is ok : R %llu C %llu", unixSeconds, utcSecondsNow);
    }
    sys.ctrl.isTimeCalibrated = true;
}

#define RTU_DEV_ID_SET    (0x1111111111111111ULL)
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
    HDL_CPU_Time_Init();
    
    MX_USB_Device_Init();
    
    LoopFrequencyTest_t loop_frq_test = {
        .measure_time = 1000, // 测试1秒钟
                              // 其他成员默认初始化为0.
    };

    uint32_t total_write_amount_bak = 0;
    mtime_t datetime;
    bool rest_task = false; // 当然重置任务标志位，当日已经执行为true,否则为false

    RTU_Sampling_Var_t rev_var;
    mtime_t rev_datetime;
    RTU_Packet_t rev_packet;
    sc_byte_buffer _4G_rev_buffer;
    sc_byte_buffer_init(&_4G_rev_buffer, _4G_rev_buf, sizeof(_4G_rev_buf));

    // HDL_ADC_Init();
    // HDL_ADC_Enable(); // 使能
    // HDL_WATCHDOG_Init(20);
    datetime_init();
    ulog_init_user();
    BFL_LED_Init();

    ULOG_INFO("====================================================================");
    ULOG_INFO("[Bootloader] Bootloader start.");
    ULOG_INFO("====================================================================");

    FatFs_Init();
    while (1) {
    }
    // App初始化

    Uart_Init(COM3, 115200, LL_LPUART_DATAWIDTH_8B, LL_LPUART_STOPBITS_1, LL_LPUART_PARITY_NONE);
    Uart_SetWriteOverCallback(COM3, releaseModbus1, NULL);
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    RS485_3_DIR_Port_CLK_EN();
    releaseModbus1();
    /**/
    GPIO_InitStruct.Pin        = RS485_3_DIR_PIN;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    LL_GPIO_Init(RS485_3_DIR_Port, &GPIO_InitStruct);
    // 初始化时释放RS485总线
    releaseModbus1();

    BFL_4G_Init("IP", "CMIOT");
    BFL_4G_TCP_Init(SOCKET0, "8.135.10.183", 32166);
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

//    while (1) {
//        uint32_t dwLen = Uart_Read(COM3, aBuf, 1000);
//        if (dwLen > 0) {
//            Uart_Write(COM1, aBuf, dwLen);
//        }
//        dwLen = Uart_Read(COM1, aBuf, 1000);
//        if (dwLen > 0) {
//            Uart_Write(COM3, aBuf, dwLen);
//        }
//    }

    uint32_t last_recv_time = 0;
    PeriodREC_t s_tims1;
    PeriodREC_t s_tims2;
    uint32_t sd_write_times = 0;
    FRESULT res;
    UINT w_buf_len           = 0;
    UINT r_buf_len           = 0;
    const char filePath[128] = {"1:data.txt"};

    int appMainStage = 0;
    while (1) {

        uint8_t sd_change = SD_CD_Has_Change();
        if (sd_change == 2) {
            ULOG_INFO("[SD Card] SD Card is removed");
            SD_Card_FatFs_DeInit();
        } else if (sd_change == 1) {
            ULOG_INFO("[SD Card] SD Card is inserted");
            SD_Card_FatFs_Init();

            snprintf((char *)filePath, sizeof(filePath), "1:test.text");
            // 尝试以 FA_OPEN_APPEND 模式打开文件
            res = f_open(&USERFile1, filePath, FA_OPEN_APPEND | FA_WRITE | FA_READ);

            if (res == FR_NO_FILE) {
                // 如果文件不存在，尝试以 FA_CREATE_NEW 模式创建文件
                res = f_open(&USERFile1, filePath, FA_CREATE_NEW | FA_WRITE | FA_READ);
            }

            if (res != FR_OK) {
                ULOG_ERROR("[FatFs] open %s failed err code = %d", filePath, res);
            } else {
                ULOG_INFO("[FatFs] open %s ok", filePath);
                f_close(&USERFile1);
            }
        }

        BFL_4G_Poll();

        uint32_t dwLen = Uart_Read(COM1, aBuf, 1000);
        if (dwLen > 0) {
            for (int i = 0; i < dwLen; i++) {
                if (aBuf[i] == '\r') {
                    Uart_Write(COM1, "\n\r", 2);
                } else {
                    Uart_Write(COM1, &aBuf[i], 1);
                }
            }
        }

        switch (appMainStage) {
            case 0: {
                if (datetime_has_synced()) {
                    appMainStage = 1;

                    // 更具当前日期创建文件
                    mtime_t datetime;
                    datetime_get_localtime(&datetime);
                    snprintf((char *)filePath, sizeof(filePath), "1:%d-%d-%d.csv", datetime.nYear, datetime.nMonth, datetime.nDay);
                    res = f_open(&USERFile1, filePath, FA_OPEN_APPEND | FA_WRITE | FA_READ);
                    if (res != FR_OK) {
                        ULOG_ERROR("[FatFs] open %s failed err code = %d", filePath, res);
                    } else {
                        ULOG_INFO("[FatFs] open %s  ok", filePath);
                    }
                }

                if (period_query_user(&s_tims1, 30)) {
                    BFL_LED_Toggle(LED1);
                }
            } break;

            case 1: {

                if (BFL_4G_TCP_Writeable(SOCKET0)) {
                    uint32_t dwLen = Uart_Read(COM3, aBuf, 50);
                    if (dwLen > 0) {
                        sc_byte_buffer_push_data(&_4G_rev_buffer, aBuf, dwLen);
                        last_recv_time = HDL_CPU_Time_GetTick();
                        //      var.datetime = 1;
                        //      var.ms = 1;
                        //      // 放数据到采样点
                        //      RTU_Sampling_Var_encoder_no_timestamp(&var, 1, &aBuf, dwLen);

                        //      if (Sensor_Queue_Write(&var) == 0)
                        //      {
                        //        ULOG_ERROR("[ETP] sample point queue is full.");
                        //      }
                    } else {
                        uint32_t dwLen1 = BFL_4G_TCP_Read(SOCKET0, RxBuf, sizeof(RxBuf));
                        if (dwLen1 > 0) {
                            takeModbus1();
                            Uart_Write(COM3, RxBuf, dwLen1);
                        }
                    }

                    if (((HDL_CPU_Time_GetTick() - last_recv_time) > 2 && sc_byte_buffer_size(&_4G_rev_buffer) > 0) ||
                        sc_byte_buffer_size(&_4G_rev_buffer) > 400) {

                        BFL_4G_TCP_Write(SOCKET0, (uint8_t *)sc_byte_buffer_data_ptr(&_4G_rev_buffer), sc_byte_buffer_size(&_4G_rev_buffer));
                        BFL_LED_Toggle(LED1);
                        // 在每次准备写入数据之前检查文件是否可写
                        res = f_write(&USERFile1, sc_byte_buffer_data_ptr(&_4G_rev_buffer), sc_byte_buffer_size(&_4G_rev_buffer), &w_buf_len);
                        if (res != FR_OK) {
                            ULOG_ERROR("[FatFs] write %s failed  err code = %d", filePath, res);
                        } else {
                            ULOG_INFO("[FatFs] write %s  ok", filePath);
                        }

                        sd_write_times++;

                        sc_byte_buffer_clear(&_4G_rev_buffer);
                    }
                }

                if (period_query_user(&s_tims2, 2000)) {
                    if (sd_write_times > 0) {
                        sd_write_times = 0;

                        // 关闭文件
                        f_close(&USERFile1);
                        // 重新打开文件
                        mtime_t datetime;
                        datetime_get_localtime(&datetime);
                        snprintf((char *)filePath, sizeof(filePath), "1:%d-%d-%d.csv", datetime.nYear, datetime.nMonth, datetime.nDay);
                        res = f_open(&USERFile1, filePath, FA_OPEN_APPEND | FA_WRITE | FA_READ);
                        if (res != FR_OK) {
                            ULOG_ERROR("[FatFs] open %s failed err code = %d", filePath, res);
                        } else {
                            ULOG_INFO("[FatFs] open %s  ok", filePath);
                        }
                    }
                }

                if (BFL_4G_TCP_Readable(SOCKET0)) {
                    uint32_t len = BFL_4G_TCP_Read(SOCKET0, (uint8_t *)buf, 80);
                    buf[len]     = '\0';
                    ULOG_INFO("Read:%s", buf);
                }

                if (SD_Card_Fs_IsMounted()) {
                    if (period_query_user(&s_tims1, 1000)) {
                        BFL_LED_Toggle(LED1);
                    }
                } else {
                    if (period_query_user(&s_tims1, 200)) {
                        BFL_LED_Toggle(LED1);
                    }
                }
            } break;
            default:
                break;
        }

        // continue;

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
    }
}
