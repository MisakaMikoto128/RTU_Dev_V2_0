/**
 * @file CHIP_W25Q512_QueueFileSystem.h
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-20
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */

#ifndef CHIP_W25Q512_QUEUEFILESYSTEM_H
#define CHIP_W25Q512_QUEUEFILESYSTEM_H

#include "CHIP_W25Q512.h"

/*
round-robin queue
FONT
↑
|----|
|----|
|----|
|----|
REAR
↑
SECTOR 0 : Header
|----|
*/

/**
 * @brief Flash中的文件信息头。
 *
 */
typedef struct tagQFSHeader
{
    uint8_t flag[4];         //"QFS"->'Q''F''S''\0' 用来简单判断是否格式化。
    uint32_t rear_sec_numb;  //队列首扇区号,下一个还没有写完的扇区的编号,是逻辑扇区号，实际写入时要加上尾部占用的一个扇区
    uint32_t rear_sec_used;  //首扇区已写入字节数0-SectorSize-1,未使用
    uint32_t font_sec_numb;  //队列尾扇区号,font_sec_numb == front_sec_numb表示为空,是逻辑扇区号，实际写入时要加上尾部占用的一个扇区
    uint32_t font_sec_poped; //首扇区已出队字节数0-SectorSize-1，在pop中使用
} QFSHeader_t;

//用户接口
void CHIP_W25Q512_QFS_init();
void CHIP_W25Q512_QFS_handler();
uint32_t CHIP_W25Q512_QFS_push(uint8_t *buf, uint32_t len);
uint32_t CHIP_W25Q512_QFS_asyn_read(uint8_t *buf, uint32_t len);
uint8_t CHIP_W25Q512_QFS_asyn_readable();
uint32_t CHIP_W25Q512_QFS_asyn_readable_byte_size();

//字节单位的QFS方法
uint32_t CHIP_W25Q512_QFS_byte_size();
uint32_t CHIP_W25Q512_QFS_pop(uint8_t *buf, uint32_t pop_len);

//扇区单位的QFS方法
uint8_t CHIP_W25Q512_QFS_is_empty();
uint8_t CHIP_W25Q512_QFS_make_empty();
uint8_t CHIP_W25Q512_QFS_is_full();
uint8_t CHIP_W25Q512_QFS_is_idle();
uint32_t CHIP_W25Q512_QFS_size();
void CHIP_W25Q512_QFS_format();
int CHIP_W25Q512_QFS_isFormated();
void CHIP_W25Q512_QFS_flush();
uint32_t CHIP_W25Q512_QFS_get_font_address();
uint32_t CHIP_W25Q512_QFS_get_rear_address();
uint32_t CHIP_W25Q512_QFS_font(uint8_t *buf);

//外部看QFS状态机
uint8_t CHIP_W25Q512_QFS_is_idle();

//测试用
extern uint32_t cache_queue_read_amount;
extern uint32_t physical_storage_read_amount;
extern uint32_t total_read_amount;
extern uint32_t total_write_amount;
extern uint64_t make_empty_amount;
/*************************Document****************************/
/*

//用户接口方法
static uint8_t test_buf[W25Q512_SECTOR_SIZE * 2];
int main()
{
    //...
    ulog_init_user();
    HDL_G4_CPU_Time_Init(); //必须启动如果使用QFS
    CHIP_W25Q512_QFS_init();
    while (1)
    {
        //...some other handler
        CHIP_W25Q512_QFS_handler();

        // Case 1:
        if (CHIP_W25Q512_QFS_asyn_readable_byte_size() > 30)
        {
            Debug_Printf("\r\n----------\r\n[QFS Test]: size before pop : %d\r\n", CHIP_W25Q512_QFS_size());
            uint32_t pop_len = 0;
            pop_len = CHIP_W25Q512_QFS_asyn_read(test_buf, 30);
            Debug_Printf("[QFS Test]: pop_len:%d\r\n", pop_len);
            test_buf[30] = 0;
            Debug_Printf("[QFS Test]: [D]:%s\r\n", test_buf);

            pop_len = CHIP_W25Q512_QFS_asyn_read(test_buf, 30);
            Debug_Printf("[QFS Test]: pop_len:%d\r\n", pop_len);
            test_buf[30] = 0;
            Debug_Printf("[QFS Test]: [D]:%s\r\n", test_buf);
            Debug_Printf("[QFS Test]: size after pop : %d\r\n", CHIP_W25Q512_QFS_size());
        }

        // Case 2:
        uint32_t len = 0;
        len = CHIP_W25Q512_QFS_asyn_read(test_buf, 30);
        if (len > 0)
        {
            test_buf[30] = 0;
            Debug_Printf("[QFS Test]: [D]:%s\r\n", test_buf);
            Debug_Printf("[QFS Test]: size after pop : %d\r\n", CHIP_W25Q512_QFS_size());
        }
    }
}

//字节单位的QFS方法
static uint8_t test_buf[W25Q512_SECTOR_SIZE * 2];
int main()
{
    //...
    ulog_init_user();
    HDL_G4_CPU_Time_Init(); //必须启动如果使用QFS
    CHIP_W25Q512_QFS_init();
    while (1)
    {
        //...some other handler
        CHIP_W25Q512_QFS_handler();

        if (CHIP_W25Q512_QFS_byte_size() > 30)
        {
            Debug_Printf("\r\n----------\r\n[QFS Test]: size before pop : %d\r\n", CHIP_W25Q512_QFS_size());
            uint32_t pop_len = 0;
            pop_len = CHIP_W25Q512_QFS_pop(test_buf, 30);
            Debug_Printf("[QFS Test]: pop_len:%d\r\n", pop_len);
            test_buf[30] = 0;
            Debug_Printf("[QFS Test]: [D]:%s\r\n", test_buf);

            CHIP_W25Q512_QFS_pop(test_buf, 30);
            pop_len = CHIP_W25Q512_QFS_pop(test_buf, 30);
            Debug_Printf("[QFS Test]: pop_len:%d\r\n", pop_len);
            test_buf[30] = 0;
            Debug_Printf("[QFS Test]: [D]:%s\r\n", test_buf);
            Debug_Printf("[QFS Test]: size after pop : %d\r\n", CHIP_W25Q512_QFS_size());
        }
    }
}
 */
/*************************Document End************************/
#endif // !CHIP_W25Q512_QUEUEFILESYSTEM_H
