/**
 * @file CHIP_W25Q512.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-09-22
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#ifndef CHIP_W25Q512_H_
#define CHIP_W25Q512_H_
#include "main.h"
#include <stdint.h>
int32_t CHIP_W25Q512_Init();
int32_t CHIP_W25Q512_read(uint32_t address, uint8_t *data, uint32_t size);
int32_t CHIP_W25Q512_write(uint32_t address, uint8_t *data, uint32_t size);

#define W25Q512_JEDEC_ID               0xEF4020UL
#define W25Q512_FLASH_SIZE             0x4000000UL
#define W25Q512_CHIP_ERASE_TIMEOUT_MAX 1000000U
#define W25Q512_PAGE_SIZE              256UL
#define W25Q512_SECTOR_SIZE            4096UL
#define W25Q512_SECTOR_COUNT           (W25Q512_FLASH_SIZE / W25Q512_SECTOR_SIZE) // 16384UL
// W25Q512默认的超时时长，单位ms。至少大于擦除一个扇区的时间50ms。
#define W25Q512_TIMEOUT_DEFAULT_VALUE 5000U
// W25Q512读取数据超时
#define W25Q512_RECEIVE_TIMEOUT 5000U

/* read register 1's bit 0 (read only), Busy flag, when erasing/writing data/writing command is set 1 */
#define W25QXX_STATUS_REG1_BUSY 0x01
/* read register 1's bit 1 (read only), WEL Write Enable Flag, when this flag is 1, it means that can be written */
#define W25QXX_STATUS_REG1_WEL 0x02
/* When ADS=0, the device is in the 3-Byte Address Mode, when ADS=1, the device is in the 4-Byte Address Mode */
#define W25QXX_STATUS_REG3_ADS 0x01
/* When ADP=0 (factory default), the device will
power up into 3-Byte Address Mode, the Extended Address Register must be used to access memory
regions beyond 128Mb. When ADP=1, the device will power up into 4-Byte Address Mode directly.
*/
#define W25QXX_STATUS_REG3_ADP 0x02
/* When WPS=0, the device is in the Write Protection Mode, when WPS=1, the device is in the Write Protection Mode */
#define W25QXX_STATUS_REG3_WPS 0x04

/* W25QXX instruction set */
#define W25QXX_CMD_WriteEnable                          0x06 /* Write Enable */
#define W25QXX_CMD_WriteEnableForVolatileStatusRegister 0x50 /* Write Enable for Volatile Status Register */
#define W25QXX_CMD_WriteDisable                         0x04 /* Write Disable */
#define W25QXX_CMD_ReadStatusReg1                       0x05 /* Read Status Register 1 */
#define W25QXX_CMD_ReadStatusReg2                       0x35 /* Read Status Register 2 */
#define W25QXX_CMD_ReadStatusReg3                       0x15 /* Read Status Register 3 */
#define W25QXX_CMD_WriteStatusReg1                      0x01 /* Write Status Register 1 */
#define W25QXX_CMD_WriteStatusReg2                      0x31 /* Write Status Register 2 */
#define W25QXX_CMD_WriteStatusReg3                      0x11 /* Write Status Register 3 */
#define W25QXX_CMD_ReadExtendedAddressReg               0xC8 /* Read Extended Address Register */
#define W25QXX_CMD_WriteExtendedAddressReg              0xC5 /* Write Extended Address Register */
#define W25QXX_CMD_Enter4ByteAddrMode                   0xB7 /* Enter 4-byte Address Mode */
#define W25QXX_CMD_Exit4ByteAddrMode                    0xE9 /* Exit 4-byte Address Mode */
#define W25QXX_CMD_ReadData                             0x03 /* Read Data */
#define W25QXX_CMD_ReadData4ByteAddr                    0x13 /* Read Data  (4-byte Address) */

#define W25QXX_CMD_FastReadData                         0x0B /* Fast Read Data */
#define W25QXX_CMD_FastReadData4ByteAddr                0x0C /* Fast Read Data (4-byte Address) */
#define W25QXX_CMD_FastReadDual                         0x3B /* Fast Read Dual Output Data */
#define W25QXX_CMD_FastReadDual4ByteAddr                0x3C /* Fast Read Dual Output Data (4-byte Address) */
#define W25QXX_CMD_FastReadQuad                         0x6B /* Fast Read Quad Output Data */
#define W25QXX_CMD_FastReadQuad4ByteAddr                0x6C /* Fast Read Quad Output Data (4-byte Address) */
#define W25QXX_CMD_FastReadDualIO                       0xBB /* Fast Read Dual I/O */
#define W25QXX_CMD_FastReadDualIO4ByteAddr              0xBC /* Fast Read Dual I/O (4-byte Address) */
#define W25QXX_CMD_FastReadQuadIO                       0xEB /* Fast Read Quad I/O */
#define W25QXX_CMD_FastReadQuadIO4ByteAddr              0xEC /* Fast Read Quad I/O (4-byte Address) */
#define W25QXX_CMD_QuadInputPageProgram                 0x32 /* Quad Input Page Program */
#define W25QXX_CMD_QuadInputPageProgram4ByteAddr        0x34 /* Quad Input Page Program (4-byte Address) */
#define W25QXX_CMD_PageProgram                          0x02 /* Page Program */
#define W25QXX_CMD_PageProgram4ByteAddr                 0x12 /* Page Program (4-byte Address) */
#define W25QXX_CMD_SectorErase                          0x20 /* Sector Erase */
#define W25QXX_CMD_SectorErase4ByteAddr                 0x21 /* Sector Erase (4-byte Address) */
#define W25QXX_CMD_BlockErase32K                        0x52 /* Block Erase 32K */
#define W25QXX_CMD_BlockErase64K                        0xD8 /* Block Erase 64K */
#define W25QXX_CMD_BlockErase64K4ByteAddr               0xDC /* Block Erase 64K 4-byte Address */
#define W25QXX_CMD_ChipErase                            0xC7 /* Chip Erase */
#define W25QXX_CMD_PowerDown                            0xB9 /* Power-Down */
#define W25QXX_CMD_ReleasePowerDown                     0xAB /* Release Power-Down */
#define W25QXX_CMD_DeviceID                             0xAB /* Read Device ID */
#define W25QXX_CMD_ManufactDeviceID                     0x90 /* Read Manufacture Device ID */
#define W25QXX_CMD_JedecDeviceID                        0x9F /* Read Jedec Device ID */
#define W25QXX_CMD_SetReadParam                         0xC0 /* Set Read Parameter */
#define W25QXX_CMD_EnterQPIMode                         0x38 /* Enter QPI Mode */
#define W25QXX_CMD_ExitQPIMode                          0xFF /* Exit QPI Mode */
#define W25QXX_CMD_EnableReset                          0x66 /* Enable Reset */
#define W25QXX_CMD_ResetDevice                          0x99 /* Reset Device */

int32_t w25q512_erase_one_sector(uint32_t sector);
int32_t w25q512_write_one_sector_no_erase(uint32_t sector, uint8_t *buf);
int32_t w25q512_write_one_sector(uint32_t sector, uint8_t *buf);
int32_t CHIP_W25Q512_read_one_sector(uint32_t sec_idx, uint8_t *buf);
#endif // !CHIP_W25Q512_H_