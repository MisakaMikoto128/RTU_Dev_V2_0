#ifndef _SHT30_H
#define _SHT30_H
#include "stm32g4xx.h"
#include "iic.h"

#define SHT30_ADD_WRITE (0X44 << 1)
#define SHT30_ADD_READ (0X44 << 1) + 1

typedef enum
{
    CMD_SOFT_RESET = 0X30A2, // software reset

    // Mode : Measurement Commands for Single Shot
    CMD_MEAS_CLOCKSTR_H = 0x2C06, // measurement: clock stretching, high repeatability
    CMD_MEAS_CLOCKSTR_M = 0x2C0D, // measurement: clock stretching, medium repeatability
    CMD_MEAS_CLOCKSTR_L = 0x2C10, // measurement: clock stretching, low repeatability

    // Mode : Measurement Commands for Periodic
    HIGH_0_5_CMD = 0x2032, // 0.5 times a second
    MEDIUM_0_5_CMD = 0x2024,
    LOW_0_5_CMD = 0x202F,
    HIGH_1_CMD = 0x2130, // 1 times a second
    MEDIUM_1_CMD = 0x2126,
    LOW_1_CMD = 0x212D,
    HIGH_2_CMD = 0x2236, // 2 times a second
    MEDIUM_2_CMD = 0x2220,
    LOW_2_CMD = 0x222B,
    HIGH_4_CMD = 0x2334, // 4 times a second
    MEDIUM_4_CMD = 0x2322,
    LOW_4_CMD = 0x2329,
    HIGH_10_CMD = 0x2737, // 10 times a second
    MEDIUM_10_CMD = 0x2721,
    LOW_10_CMD = 0x272A,

    // Command to read data in periodic measurement mode
    READOUT_FOR_PERIODIC_MODE = 0xE000,

} SHT30;

typedef struct
{
    uint16_t temp;
    uint16_t humi;

} sht30Var_t;

void CHIP_SHT30_Init(void);

void CHIP_SHT30_Read(sht30Var_t *pSht30Var);

void CHIP_SHT30_Write(uint16_t CMD);

/*
空闲态->传输中->等待处理->[空闲态]
*/

uint8_t CHIP_SHT30_request();
uint8_t CHIP_SHT30_query_event();
uint8_t CHIP_SHT30_query_result();
void CHIP_SHT30_clear_all_event();
void CHIP_SHT30_get_result(sht30Var_t *pSht30Var);
void CHIP_SHT30_handler();
#endif
