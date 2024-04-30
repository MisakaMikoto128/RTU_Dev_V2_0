/**
 * @file CHIP_W25Q512.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-09-22
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "CHIP_W25Q512.h"

QSPI_HandleTypeDef hqspi1;
#define w25qxx_hqspi hqspi1

// 将x按照n对齐，例如ALIGN(3,4) = 4，ALIGN(6,4) = 8。n必须为2的幂次。
#define ALIGN(x, n) = (((x) + (n) - 1) & ~((n) - 1))
// 计算log2(VAL)，VAL必须为2的幂次
#define POSITION_VAL(VAL) (__CLZ(__RBIT(VAL)))
// 大小为W25Q512一个Sector的缓存区，用于CHIP_W25Q512_write方法，在擦除一个扇区时缓存其数据。
static __IO uint8_t w25q512_buf[W25Q512_SECTOR_SIZE] = {0};
int32_t w25q512_send_cmd(uint8_t cmd);
int32_t w25q512_wait_busy(uint32_t timeout);
int32_t w25q512_erase_one_sector(uint32_t sector);
int32_t w25q512_write_page_no_erase(uint32_t address, uint8_t *buf, uint32_t size);
int32_t w25q512_write_one_sector_no_erase(uint32_t sector, uint8_t *buf);
int32_t w25q512_write_one_sector(uint32_t sector, uint8_t *buf);
uint8_t w25q512_read_status_reg(uint8_t reg);
uint8_t w25q512_is_busy();
int32_t w25q512_erase_one_sector_cmd(uint32_t sector);
/**
 * @brief 初始化W25Q512，使得芯片能够正常读写。
 *
 * @return int32_t 成功返回0，失败返回-1
 * @note 1. 初始化QSPI。2.复位W25Q512。2.使得W25Q512工作在4字节模式。
 */
int32_t CHIP_W25Q512_Init()
{
    int32_t status = 0;
    // 1.启动QSPI外设
    w25qxx_hqspi.Instance = QUADSPI;
    HAL_QSPI_DeInit(&w25qxx_hqspi); // reset QSPI

    w25qxx_hqspi.Instance                = QUADSPI;
    w25qxx_hqspi.Init.ClockPrescaler     = 4 - 1;
    w25qxx_hqspi.Init.FifoThreshold      = 8;
    w25qxx_hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    w25qxx_hqspi.Init.FlashSize          = POSITION_VAL(W25Q512_FLASH_SIZE) + 1 - 1;
    w25qxx_hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
    w25qxx_hqspi.Init.ClockMode          = QSPI_CLOCK_MODE_0;
    w25qxx_hqspi.Init.FlashID            = QSPI_FLASH_ID_1;
    w25qxx_hqspi.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;
    if (HAL_QSPI_Init(&w25qxx_hqspi) != HAL_OK) {
        status = -1;
    }

    // 2. 重置、进入4字节地址模式
    w25q512_send_cmd(W25QXX_CMD_EnableReset);
    w25q512_send_cmd(W25QXX_CMD_ResetDevice);
    w25q512_send_cmd(W25QXX_CMD_Exit4ByteAddrMode);
    w25q512_send_cmd(W25QXX_CMD_Enter4ByteAddrMode);
    return status;
}

/**
 * @brief 从flash芯片随机读取一定数量的数据到内存。
 *
 * @param address 从Flash读取数据的地址。
 * @param buf 指向待存放数据的指针。
 * @param size 需要读取数据的长度，单位字节。
 * @return int32_t
 */
int32_t CHIP_W25Q512_read(uint32_t address, uint8_t *buf, uint32_t size)
{
    int32_t status = 0;
    QSPI_CommandTypeDef s_command;
    QSPI_CommandTypeDef *pcmd = &s_command;
    // pcmd->InstructionMode = QSPI_INSTRUCTION_1_LINE;
    // pcmd->AddressSize = QSPI_ADDRESS_32_BITS;
    // pcmd->DdrMode = QSPI_DDR_MODE_DISABLE;
    // pcmd->DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    // pcmd->SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD; //第一次发送数据的时候发送指令，还没连续读取
    // pcmd->AddressMode = QSPI_ADDRESS_1_LINE;
    // pcmd->DataMode = QSPI_DATA_4_LINES;
    // pcmd->DummyCycles = 8; //取决于指令W25QXX_CMD_FastReadQuad4ByteAddr
    // pcmd->NbData = size;
    // pcmd->Address = address;
    // pcmd->Instruction = W25QXX_CMD_FastReadQuad4ByteAddr;
    // pcmd->AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    // pcmd->AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    // pcmd->AlternateBytes = 0xFF;

    // 指令也四线
    pcmd->InstructionMode    = QSPI_INSTRUCTION_1_LINE;
    pcmd->AddressSize        = QSPI_ADDRESS_32_BITS;
    pcmd->DdrMode            = QSPI_DDR_MODE_DISABLE;
    pcmd->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    pcmd->SIOOMode           = QSPI_SIOO_INST_ONLY_FIRST_CMD;
    pcmd->AddressMode        = QSPI_ADDRESS_4_LINES;
    pcmd->DataMode           = QSPI_DATA_4_LINES;
    pcmd->DummyCycles        = 4;
    pcmd->NbData             = size;
    pcmd->Address            = address;
    pcmd->Instruction        = W25QXX_CMD_FastReadQuadIO4ByteAddr; // 1-4-4模式下(1线指令4线地址4线数据)，快速读取指令
    pcmd->AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    pcmd->AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
    pcmd->AlternateBytes     = 0xFF;

    // pcmd->Instruction = W25QXX_CMD_FastReadDual4ByteAddr;
    // pcmd->Address = address;
    // pcmd->DummyCycles = 8;
    // pcmd->AddressSize = QSPI_ADDRESS_32_BITS;
    // pcmd->DataMode = QSPI_DATA_2_LINES;
    // pcmd->InstructionMode = QSPI_INSTRUCTION_1_LINE;
    // pcmd->AddressMode = QSPI_ADDRESS_1_LINE;
    // pcmd->DdrMode = QSPI_DDR_MODE_DISABLE;
    // pcmd->DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    // pcmd->SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    // pcmd->AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    // pcmd->NbData = size;

    // pcmd->Instruction = W25QXX_CMD_FastReadData4ByteAddr;
    // pcmd->Address = address;
    // pcmd->DummyCycles = 8;
    // pcmd->AddressSize = QSPI_ADDRESS_32_BITS;
    // pcmd->DataMode = QSPI_DATA_1_LINE;
    // pcmd->InstructionMode = QSPI_INSTRUCTION_1_LINE;
    // pcmd->AddressMode = QSPI_ADDRESS_1_LINE;
    // pcmd->DdrMode = QSPI_DDR_MODE_DISABLE;
    // pcmd->DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    // pcmd->SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
    // pcmd->AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    // pcmd->NbData = size;

    // pcmd->Instruction = W25QXX_CMD_ReadData4ByteAddr;
    // pcmd->Address = address;
    // pcmd->DummyCycles = 8;
    // pcmd->NbData = size;
    // pcmd->AddressSize = QSPI_ADDRESS_32_BITS;
    // pcmd->DataMode = QSPI_DATA_1_LINE;
    // pcmd->InstructionMode = QSPI_INSTRUCTION_1_LINE;
    // pcmd->AddressMode = QSPI_ADDRESS_1_LINE;
    // pcmd->DdrMode = QSPI_DDR_MODE_DISABLE;
    // pcmd->DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    // pcmd->SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    // pcmd->AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;

    if (HAL_QSPI_Command(&w25qxx_hqspi, &s_command, W25Q512_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        status = -1;
        return status;
    }

    if (HAL_QSPI_Receive(&w25qxx_hqspi, buf, W25Q512_RECEIVE_TIMEOUT) != HAL_OK) {
        status = -1;
        return status;
    }

    // 这一句是必要的
    w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);
    return status;
}

/**
 * @brief 随机写一定数量的数据flash芯片。
 *
 * @param address 数据写到flash中的地址。
 * @param data 指向待存放数据的指针。
 * @param size 数据长度，单位字节。
 * @return int32_t 成功返回0，失败返回-1。
 * @note 其实现依赖于一个全局的缓存区，首先会将数据读取到缓存区，然后判断该扇区是否需要擦除，
 * 最后将需要写的数据添加到缓存区，再写入Flash。如果写入数据较少，那么至少会花去擦除一个扇区
 * 的时间。W25Q512擦除一个扇区的时间大约为50ms。
 */
int32_t CHIP_W25Q512_write(uint32_t address, uint8_t *data, uint32_t size)
{
    int32_t status = 0;
    // 1.计算起始写入的扇区编号和起始写入扇区剩余需要写入的字节数量。
    uint32_t current_sec_waiting_to_write_byte_num = 0; // The number of bytes waiting to be written
    uint32_t current_sec_remain_bytes              = 0; // 当前扇区剩余需要写入字节的数量
    uint32_t sec                                   = 0;
    uint32_t current_sec_start_write_addr          = 0; // 当前扇区写入数据的相对起始地址
    sec                                            = address / W25Q512_SECTOR_SIZE;

    // 计算的一个扇区剩余需要写入字节的数量
    current_sec_remain_bytes = W25Q512_SECTOR_SIZE - address % W25Q512_SECTOR_SIZE;

    if (size <= current_sec_remain_bytes) {
        current_sec_waiting_to_write_byte_num = size;
    } else {
        current_sec_waiting_to_write_byte_num = current_sec_remain_bytes;
    }

    while (1) {
        CHIP_W25Q512_read(sec * W25Q512_SECTOR_SIZE, (uint8_t *)w25q512_buf, W25Q512_SECTOR_SIZE);
        current_sec_start_write_addr = address % W25Q512_SECTOR_SIZE;
        for (uint32_t i = 0; i < current_sec_waiting_to_write_byte_num; i++) {
            w25q512_buf[current_sec_start_write_addr + i] = data[i];
        }
        w25q512_erase_one_sector(sec);
        w25q512_write_one_sector_no_erase(sec, (uint8_t *)w25q512_buf);

        size -= current_sec_waiting_to_write_byte_num;
        if (size == 0) {
            break;
        }

        address += current_sec_waiting_to_write_byte_num;
        data += current_sec_waiting_to_write_byte_num;
        if (size > W25Q512_SECTOR_SIZE) {
            current_sec_waiting_to_write_byte_num = W25Q512_SECTOR_SIZE;
        } else {
            current_sec_waiting_to_write_byte_num = size;
        }
        sec++;
    }
    return status;
}

/**
 * @brief 等待芯片BUSY状态结束。每次执行Page Program, Quad Page Program, Sector Erase,
 * Block Erase, Chip Erase, Write Status Register or Erase/Program Security Register
 * 指令后W25Q512会进入BUSY状态，需要等待BUSY状态结束才能继续下一步操作。
 *
 * @param timeout 等待超时时长
 * @return int32_t 未超时返回0，超时返回-1
 */
int32_t w25q512_wait_busy(uint32_t timeout)
{
    int32_t status = 0;

    QSPI_CommandTypeDef s_command;
    QSPI_AutoPollingTypeDef s_config;

    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.Instruction       = W25QXX_CMD_ReadStatusReg1;

    s_config.Match           = 0;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.Interval        = 0x10; // 这里设置轮询时间
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;
    s_config.StatusBytesSize = 1;
    s_config.Mask            = W25QXX_STATUS_REG1_BUSY;

    if (HAL_QSPI_AutoPolling(&w25qxx_hqspi, &s_command, &s_config, timeout) != HAL_OK) {
        status = -1;
    }
    return status;
}

/**
 * @brief 向W25Q512发送指令。但是只支持不包含地址也不需要读取数据的指令。
 *
 * @param cmd 命令。
 * @return int32_t 成功返回0，失败返回-1。
 */
int32_t w25q512_send_cmd(uint8_t cmd)
{
    QSPI_CommandTypeDef qspi_handler = {0};
    int32_t status                   = 0;
    qspi_handler.Instruction         = cmd;
    qspi_handler.Address             = 0;
    qspi_handler.DummyCycles         = 0;
    qspi_handler.InstructionMode     = QSPI_INSTRUCTION_1_LINE;
    qspi_handler.AddressMode         = QSPI_ADDRESS_NONE;
    qspi_handler.AddressSize         = QSPI_ADDRESS_32_BITS;
    qspi_handler.DataMode            = QSPI_DATA_NONE;
    qspi_handler.SIOOMode            = QSPI_SIOO_INST_EVERY_CMD;
    qspi_handler.AlternateByteMode   = QSPI_ALTERNATE_BYTES_NONE;
    qspi_handler.DdrMode             = QSPI_DDR_MODE_DISABLE;
    qspi_handler.DdrHoldHalfCycle    = QSPI_DDR_HHC_ANALOG_DELAY;

    if (HAL_QSPI_Command(&w25qxx_hqspi, &qspi_handler, W25Q512_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        status = -1;
    }
    return status;
}

/**
 * @brief W25Q512擦除一个扇区。
 *
 * @param sector 扇区编号。
 * @return int32_t 成功返回0，失败返回-1。
 */
int32_t w25q512_erase_one_sector(uint32_t sector)
{
    int32_t status   = 0;
    uint32_t address = 0;
    QSPI_CommandTypeDef qspi_handler;

    address = sector * W25Q512_SECTOR_SIZE;
    w25q512_send_cmd(W25QXX_CMD_WriteEnable);
    w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);

    qspi_handler.Instruction       = W25QXX_CMD_SectorErase4ByteAddr;
    qspi_handler.Address           = address;
    qspi_handler.DummyCycles       = 0;
    qspi_handler.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    qspi_handler.AddressMode       = QSPI_ADDRESS_1_LINE;
    qspi_handler.AddressSize       = QSPI_ADDRESS_32_BITS;
    qspi_handler.DataMode          = QSPI_DATA_NONE;
    qspi_handler.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    qspi_handler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    qspi_handler.DdrMode           = QSPI_DDR_MODE_DISABLE;
    qspi_handler.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    if (HAL_QSPI_Command(&w25qxx_hqspi, &qspi_handler, W25Q512_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        status = -1;
    }
    w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);
    return status;
}

/**
 * @brief 发送W25Q512擦除一个扇区指令，不做等待。
 *
 * @param sector 扇区编号。
 * @return int32_t 成功返回0，失败返回-1。
 */
int32_t w25q512_erase_one_sector_cmd(uint32_t sector)
{
    int32_t status   = 0;
    uint32_t address = 0;
    QSPI_CommandTypeDef qspi_handler;

    address = sector * W25Q512_SECTOR_SIZE;
    w25q512_send_cmd(W25QXX_CMD_WriteEnable);
    w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);

    qspi_handler.Instruction       = W25QXX_CMD_SectorErase4ByteAddr;
    qspi_handler.Address           = address;
    qspi_handler.DummyCycles       = 0;
    qspi_handler.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    qspi_handler.AddressMode       = QSPI_ADDRESS_1_LINE;
    qspi_handler.AddressSize       = QSPI_ADDRESS_32_BITS;
    qspi_handler.DataMode          = QSPI_DATA_NONE;
    qspi_handler.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    qspi_handler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    qspi_handler.DdrMode           = QSPI_DDR_MODE_DISABLE;
    qspi_handler.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    if (HAL_QSPI_Command(&w25qxx_hqspi, &qspi_handler, W25Q512_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        status = -1;
    }
    return status;
}

/**
 * @brief W25Q512写入一个页的数据。_no_erase表示不进行擦除。
 * @note W25Q512有256字节的页缓冲区，写数据的起始地址可以是随机的，不需要考虑对齐，
 * 但是必须预先擦除，以确保被写入的字节单元写入数据前的值为0xFF。W25Q512一次最多写
 * 一页数据。
 *
 * @param buf 指向待写入数据的指针。
 * @param address 写入闪存的数据地址。
 * @param size 待写入数据的大小，单位字节。
 * @return int32_t 成功返回0，失败返回-1。
 */
int32_t w25q512_write_page_no_erase(uint32_t address, uint8_t *buf, uint32_t size)
{
    int32_t status = 0;
    QSPI_CommandTypeDef s_command;
    w25q512_send_cmd(W25QXX_CMD_WriteEnable);
    w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);

    s_command.Instruction       = W25QXX_CMD_QuadInputPageProgram4ByteAddr;
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_32_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_ONLY_FIRST_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = 0;
    s_command.NbData            = size;
    s_command.Address           = address;

    // 如果QSPI的频率太高，而你使用的是杜邦线，那么可以实时单线SPI模式
    //  s_command.Instruction = W25QXX_CMD_PageProgram4ByteAddr;
    //  s_command.Address = address;
    //  s_command.DummyCycles = 0;
    //  s_command.NbData = size;
    //  s_command.AddressSize = QSPI_ADDRESS_32_BITS;
    //  s_command.DataMode = QSPI_DATA_1_LINE;
    //  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    //  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    //  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    //  s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    //  s_command.SIOOMode = QSPI_SIOO_INST_ONLY_FIRST_CMD;
    //  s_command.AddressMode = QSPI_ADDRESS_1_LINE;

    if (HAL_QSPI_Command(&w25qxx_hqspi, &s_command, W25Q512_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        status = -1;
        return status;
    }

    if (HAL_QSPI_Transmit(&w25qxx_hqspi, buf, W25Q512_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        status = -1;
        return status;
    }
    w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);
    return status;
}

/**
 * @brief 删除了等待忙碌，功能与w25q512_write_page_no_erase完全相同。
 *
 * @param address
 * @param buf
 * @param size
 * @return int32_t
 */
int32_t w25q512_write_page_no_erase_no_wait(uint32_t address, uint8_t *buf, uint32_t size)
{
    int32_t status = 0;
    QSPI_CommandTypeDef s_command;
    w25q512_send_cmd(W25QXX_CMD_WriteEnable);
    w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);

    s_command.Instruction       = W25QXX_CMD_QuadInputPageProgram4ByteAddr;
    s_command.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command.AddressSize       = QSPI_ADDRESS_32_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode          = QSPI_SIOO_INST_ONLY_FIRST_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = 0;
    s_command.NbData            = size;
    s_command.Address           = address;

    if (HAL_QSPI_Command(&w25qxx_hqspi, &s_command, W25Q512_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        status = -1;
        return status;
    }

    if (HAL_QSPI_Transmit(&w25qxx_hqspi, buf, W25Q512_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        status = -1;
        return status;
    }
    return status;
}

/**
 * @brief 向指定扇区写入一个扇区大小的数据。_no_erase表示不进行擦除。
 *
 * @param buf 指向待写入数据的指针。
 * @param sector 扇区编号。
 * @return int32_t 成功返回0，失败返回-1。
 */
int32_t w25q512_write_one_sector_no_erase(uint32_t sector, uint8_t *buf)
{
    int32_t status = 0;

    for (int page_idx = 0; page_idx < W25Q512_SECTOR_SIZE / W25Q512_PAGE_SIZE; page_idx++) {
        status = w25q512_write_page_no_erase(sector * W25Q512_SECTOR_SIZE + page_idx * W25Q512_PAGE_SIZE, buf + page_idx * W25Q512_PAGE_SIZE, W25Q512_PAGE_SIZE);
    }
    return status;
}

/**
 * @brief 向指定扇区写入一个扇区大小的数据，会擦除原来的数据。
 *
 * @param sector 扇区编号。
 * @param buf 指向待写入数据的指针,其中至少包含一个W25Q512_SECTOR_SIZE大小的数据
 * @return int32_t 成功返回0，失败返回-1。
 */
int32_t w25q512_write_one_sector(uint32_t sector, uint8_t *buf)
{
    int32_t status = 0;

    w25q512_erase_one_sector(sector);
    status = w25q512_write_one_sector_no_erase(sector, buf);
    return status;
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *qspiHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (qspiHandle->Instance == QUADSPI) {
        /* USER CODE BEGIN QUADSPI_MspInit 0 */

        /* USER CODE END QUADSPI_MspInit 0 */
        LL_RCC_SetQUADSPIClockSource(LL_RCC_QUADSPI_CLKSOURCE_SYSCLK);
        // LL_RCC_SetQUADSPIClockSource(LL_RCC_QUADSPI_CLKSOURCE_PLL);
        /* QUADSPI clock enable */
        __HAL_RCC_QSPI_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**QUADSPI1 GPIO Configuration
        PA2     ------> QUADSPI1_BK1_NCS
        PA3     ------> QUADSPI1_CLK
        PA6     ------> QUADSPI1_BK1_IO3
        PA7     ------> QUADSPI1_BK1_IO2
        PB0     ------> QUADSPI1_BK1_IO1
        PB1     ------> QUADSPI1_BK1_IO0
        */
        /**QUADSPI1 GPIO Pull
               QSPI_CLOCK_MODE_0->CLK IDLE IS LOW
               QSPI_CLOCK_MODE_3->CLK IDLE IS HIGH
               CS->IDLE IS HIGH
        */
        GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin       = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* USER CODE BEGIN QUADSPI_MspInit 1 */

        /* USER CODE END QUADSPI_MspInit 1 */
    }
}

void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef *qspiHandle)
{

    if (qspiHandle->Instance == QUADSPI) {
        /* USER CODE BEGIN QUADSPI_MspDeInit 0 */

        /* USER CODE END QUADSPI_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_QSPI_CLK_DISABLE();

        /**QUADSPI1 GPIO Configuration
        PA2     ------> QUADSPI1_BK1_NCS
        PA3     ------> QUADSPI1_CLK
        PA6     ------> QUADSPI1_BK1_IO3
        PA7     ------> QUADSPI1_BK1_IO2
        PB0     ------> QUADSPI1_BK1_IO1
        PB1     ------> QUADSPI1_BK1_IO0
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_6 | GPIO_PIN_7);

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0 | GPIO_PIN_1);

        /* USER CODE BEGIN QUADSPI_MspDeInit 1 */

        /* USER CODE END QUADSPI_MspDeInit 1 */
    }
}

/**
 * @brief W25QXX read status register.
 * @brief The explain of the read register.
 * status register 1(default value 0x00):
 * 7   6  5  4   3   2   1   0
 * SPR RV TB BP2 BP1 BP0 WEL BUSY
 * SPR default is 0,the protection of the status register,use with WP.
 * TB,BP2,BP1,BP0 flash area write protection setting bit.
 * WEL write enable lock.
 * BUSY busy flag,BUSY = 1 is busy,BUSY = 0 is idle.
 * status register 2:
 * 7   6   5   4   3   2   1  0
 * SUS CMP LB3 LB2 LB1 (R) QE SPR1
 * status register 3:
 * 7        6    5    4   3   2   1   0
 * HOLD/RST BRV1 DRV0 (R) (R) WPS ADP ADS
 * @param reg It could be W25QXX_CMD_ReadStatusReg1~3,represents the 8-bit status registers 1,2,3.
 * @return uint8_t the value of the status register.
 */
uint8_t w25q512_read_status_reg(uint8_t reg)
{
    uint8_t byte, cmd;
    cmd = reg;
    QSPI_CommandTypeDef qspi_handler;
    qspi_handler.Instruction       = cmd;
    qspi_handler.Address           = 0;
    qspi_handler.DummyCycles       = 0;
    qspi_handler.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    qspi_handler.AddressMode       = QSPI_ADDRESS_NONE;
    qspi_handler.AddressSize       = QSPI_ADDRESS_24_BITS;
    qspi_handler.DataMode          = QSPI_DATA_1_LINE;
    qspi_handler.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    qspi_handler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    qspi_handler.DdrMode           = QSPI_DDR_MODE_DISABLE;
    qspi_handler.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    qspi_handler.NbData            = 1;
    HAL_QSPI_Command(&w25qxx_hqspi, &qspi_handler, W25Q512_TIMEOUT_DEFAULT_VALUE);
    HAL_QSPI_Receive(&w25qxx_hqspi, &byte, W25Q512_TIMEOUT_DEFAULT_VALUE);
    return byte;
}

/**
 * @brief 查看W25Q512是否处于BUSY状态。
 *
 * @return uint8_t 忙碌BUSY 1,空闲0
 */
uint8_t w25q512_is_busy()
{
    uint8_t status = 0;
    uint8_t reg1   = w25q512_read_status_reg(W25QXX_CMD_ReadStatusReg1);
    status         = (uint8_t)((reg1 & W25QXX_STATUS_REG1_BUSY) == W25QXX_STATUS_REG1_BUSY);
    return status;
}