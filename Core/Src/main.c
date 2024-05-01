/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "usart.h"
#include "quadspi.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usb.h"

#include "HDL_Uart.h"
#include "CHIP_W25Q512_test.h"
#include "HDL_Uart_test.h"
#include "HDL_RTC_test.h"
#include "HDL_CPU_Time_test.h"
#include "HDL_Flash_test.h"
#include "log.h"
#include "modbus_test.h"
#include "CHIP_SHT30_test.h"
#include "HDL_ADC_test.h"
#include "scheduler_test.h"
#include "APP_RTU_Sampler_test.h"
#include "APP_Main.h"
#include "BFL_RTU_Packet_test.h"

#include "HDL_Uart.h"
#include "HDL_RTC.h"
#include "HDL_CPU_Time.h"

#include "log.h"
#include <string.h>
#include <stdio.h>
#include "sensor_test.h"
#include "BFL_LED.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    //  MX_I2C1_Init();
    //  MX_LPUART1_UART_Init();
    //  MX_UART4_Init();
    //  MX_UART5_Init();
    //  MX_USART1_UART_Init();
    //  MX_USART2_UART_Init();
    //  MX_DMA_Init();
    //  MX_QUADSPI1_Init();
    //  MX_ADC1_Init();
    //  MX_ADC2_Init();
    //  MX_IWDG_Init();
    //  MX_RTC_Init();
    //  MX_SPI1_Init();
    //  MX_TIM1_Init();
    //  MX_USB_PCD_Init();
    //  MX_USART3_UART_Init();
    //  MX_TIM2_Init();
    /* USER CODE BEGIN 2 */
    // W25Q512测试
    // ulog_init_user();
    // CHIP_W25Q512_Init();

    // while (true) {
    //     CHIP_W25Q512_io_rate();
    //     CHIP_W25Q512_io_check(12);
    //     CHIP_W25Q512_io_check(W25Q512_SECTOR_SIZE / 4);
    //     CHIP_W25Q512_io_check(W25Q512_SECTOR_SIZE * 100 + W25Q512_SECTOR_SIZE / 4);
    //     CHIP_W25Q512_io_check(W25Q512_SECTOR_SIZE * 200 + W25Q512_SECTOR_SIZE / 4);
    //     CHIP_W25Q512_sector_io_check();
    // }

    //   CHIP_W25Q512_QFS_test();
    // 串口测试
    // HDL_Uart_fifo_test_loop();
    // HDL_Uart_test_loop();
    // RTC测试
    // HDL_RTC_test();
    // HDL_RTC_leap_year_test();
    // HDL_RTC_subsecond_test();
    // CPU tick测试
    // HDL_CPU_Time_test();
    // HDL_CPU_Time_hard_timer_test();
    // Flash测试
    // Flash_inner_fun_test();
    // Flash_test();
    // Modbus测试

    // modbus_rtu_host_test();
    // modbus_rtu_host_test2();
    // modbus_rtu_host_sensor_test();
    // modbus_rtu_host_sensor_test2();
    // modbus_rtu_host_rs485_3_test();

    // SHT30测试
    // CHIP_SHT30_poll_test();
    // CHIP_SHT30_async_test();

    // ADC测试
    // HDL_ADC_test();

    // Scheduler 测试
    // scheduler_test();
    // period_test();

    // APP_Sampler测试
    // APP_RTU_Sampler_test();
    // APP_RTU_Sampler_encoder_test();
    // RTU数据包打包程序测试
    // BFL_RTU_Packet_test();
    // 光伏RTU主程序处理器。
    // sensor_testt();

    // BFL_LED_Init();
    // while (1)
    // {
    //   for (LED_t led = LED1; led < LED_NUM; led++)
    //   {
    //     BFL_LED_Toggle(led);
    //     HDL_CPU_Time_DelayMs(1000);
    //   }
    // }

    APP_Main();
    // APP_Main_test();
    // RTU综合测试
    // rtu_test();
    // 4G模块测试
    // BFL_4G_test();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4) {
    }
    LL_PWR_EnableRange1BoostMode();
    LL_RCC_HSE_Enable();
    /* Wait till HSE is ready */
    while (LL_RCC_HSE_IsReady() != 1) {
    }

    LL_RCC_HSI48_Enable();
    /* Wait till HSI48 is ready */
    while (LL_RCC_HSI48_IsReady() != 1) {
    }

    LL_RCC_LSI_Enable();
    /* Wait till LSI is ready */
    while (LL_RCC_LSI_IsReady() != 1) {
    }

    LL_PWR_EnableBkUpAccess();
    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
    LL_RCC_LSE_Enable();
    /* Wait till LSE is ready */
    while (LL_RCC_LSE_IsReady() != 1) {
    }

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_2, 85, LL_RCC_PLLR_DIV_2);
    LL_RCC_PLL_EnableDomain_SYS();
    LL_RCC_PLL_Enable();
    /* Wait till PLL is ready */
    while (LL_RCC_PLL_IsReady() != 1) {
    }

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);
    /* Wait till System clock is ready */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {
    }

    /* Insure 1?s transition state at intermediate medium speed clock based on DWT */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;
    while (DWT->CYCCNT < 100);
    /* Set AHB prescaler*/
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_SetSystemCoreClock(170000000);

    /* Update the time base */
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM1) {
        // uwCpuTick++;
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
