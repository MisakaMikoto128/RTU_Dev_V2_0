#include "log.h"
#include <stdarg.h>
#include "HDL_CPU_Time.h"
#include "HDL_RTC.h"
#include "datetime.h"
#include "HDL_Uart.h"
#include <stdio.h>

// #undef FREE_RTOS
#ifdef FREE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#endif // FREE_RTOS

// 调试输出数据包最大长度
#define MAXDEBUGSEND 256
static uint8_t buffer[MAXDEBUGSEND + 1];
#define DEBUG_COM COM1
/**
 * @brief 使用串口调试的格式化输出方法
 *
 * @param format
 * @param ...
 */
void Debug_Printf(const void *format, ...)
{
#ifdef ULOG_ENABLED
    uint32_t uLen;
    va_list vArgs;
    va_start(vArgs, format);
    uLen = vsnprintf((char *)buffer, MAXDEBUGSEND, (char const *)format, vArgs);
    va_end(vArgs);
    if (uLen > MAXDEBUGSEND)
        uLen = MAXDEBUGSEND;
#if USING_RTT == 1
    SEGGER_RTT_Write(0, buffer, uLen);
#elif USING_USB_CDC == 1
    CDC_Transmit_FS(buffer, uLen);
#elif USING_UART == 1
    Uart_Write(DEBUG_COM, buffer, uLen);
#endif
#endif // ULOG_ENABLED
}

void my_console_logger(ulog_level_t severity, char *msg)
{
#ifdef ULOG_ENABLED
#ifdef FREE_RTOS
    taskENTER_CRITICAL();
#endif // FREE_RTOS
    mtime_t mtime;
    datetime_get_localtime(&mtime);

    switch (severity) {
        case ULOG_WARNING_LEVEL:
            Debug_Printf(">%04d.%02d.%02d %02d:%02d:%02d  [%s]: %s\r\n",
                         mtime.nYear, mtime.nMonth, mtime.nDay, mtime.nHour, mtime.nMin, mtime.nSec,
                         ulog_level_name(severity),
                         msg);
            break;
        case ULOG_ERROR_LEVEL:
            Debug_Printf(">%04d.%02d.%02d %02d:%02d:%02d  [%s]: %s\r\n",
                         mtime.nYear, mtime.nMonth, mtime.nDay, mtime.nHour, mtime.nMin, mtime.nSec,
                         ulog_level_name(severity),
                         msg);
            break;
        default:
            Debug_Printf(">%04d.%02d.%02d %02d:%02d:%02d  [%s]: %s\r\n",
                         mtime.nYear, mtime.nMonth, mtime.nDay, mtime.nHour, mtime.nMin, mtime.nSec,
                         ulog_level_name(severity),
                         msg);
            break;
    }
#ifdef FREE_RTOS
    taskEXIT_CRITICAL();
#endif // FREE_RTOS
#endif // ULOG_ENABLED
}

void ulog_init_user()
{
    ULOG_INIT();

#if USING_RTT == 1
    /* 配置通道 0，上行配置*/
    SEGGER_RTT_ConfigUpBuffer(0, "RTTUP", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    /* 配置通道 0，下行配置*/
    // SEGGER_RTT_ConfigDownBuffer(0, "RTTDOWN", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    SEGGER_RTT_SetTerminal(0);
#elif USING_USB_CDC == 1
#elif USING_UART == 1
    Uart_Init(DEBUG_COM, 1500000UL, LL_USART_DATAWIDTH_8B, LL_USART_STOPBITS_1, LL_USART_PARITY_NONE);
#endif
    // dynamically change the threshold for a specific logger
    ULOG_SUBSCRIBE(my_console_logger, ULOG_DEBUG_LEVEL);
}