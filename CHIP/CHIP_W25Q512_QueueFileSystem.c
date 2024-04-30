/**
 * @file CHIP_W25Q512_QueueFileSystem.c
 * @author Liu Yuanlin (liuyuanlins@outlook.com)
 * @brief
 * @version 0.1
 * @date 2022-10-21
 *
 * @copyright Copyright (c) 2022 Liu Yuanlin Personal.
 *
 */
#include "CHIP_W25Q512_QueueFileSystem.h"
#include "HDL_CPU_Time.h"
#include <stdlib.h>
#include <string.h>
#include "circular_array_queu.h"

int32_t w25q512_send_cmd(uint8_t cmd);
int32_t w25q512_wait_busy(uint32_t timeout);
int32_t w25q512_erase_one_sector(uint32_t sector);
int32_t w25q512_write_page_no_erase(uint32_t address, uint8_t *buf, uint32_t size);
int32_t w25q512_write_one_sector_no_erase(uint32_t sector, uint8_t *buf);
int32_t w25q512_write_page_no_erase_no_wait(uint32_t address, uint8_t *buf, uint32_t size);
int32_t w25q512_write_one_sector(uint32_t sector, uint8_t *buf);
uint8_t w25q512_read_status_reg(uint8_t reg);
uint8_t w25q512_is_busy();
int32_t w25q512_erase_one_sector_cmd(uint32_t sector);

int32_t CHIP_W25Q512_read_one_sector(uint32_t sec_idx, uint8_t *buf);

static int CHIP_W25Q512_QFS_start_flush_header();
uint32_t CHIP_W25Q512_QFS_asyn_pop(uint8_t *buf, uint32_t pop_len);

/* 定义逻辑扇区号到物理扇区号的映射 */
/* INDEX : 索引号， COUNT : 数量 */

// QFS头部信息存储扇区大小,一个扇区
#define QFS_HEADER_SECTOR_SIZE 1
// QFS数据区域逻辑扇区数量,最大为W25Q512_SECTOR_COUNT - 1UL，有一个扇区用于存储头部信息。
#define QFS_DATA_FILED_LOGIC_SECTOR_COUNT (W25Q512_SECTOR_COUNT / 4 - 1UL)
// 逻辑扇区号到物理扇区号的偏移.例如这个偏移量等于1时表示QFS从物理扇区编号1开始存储数据
#define LOGIC_SECTOR_OFFSET_OF_PHYSICAL_SECTOR_INDEX (W25Q512_SECTOR_COUNT / 4)

#define QFS_HEADER_PHYSICAL_SECTOR_INDEX             (LOGIC_SECTOR_OFFSET_OF_PHYSICAL_SECTOR_INDEX)
// QFS的数据存储区域始终开始于首部信息存储扇区之后的一个扇区，也就是物理扇区索引@QFS_HEADER_PHYSICAL_SECTOR_INDEX + 1的位置
#define QFS_PHYSICAL_SECTOR_INDEX(LOGIC_SECTOR_NUMB) ((LOGIC_SECTOR_NUMB) + QFS_HEADER_PHYSICAL_SECTOR_INDEX + QFS_HEADER_SECTOR_SIZE)

typedef enum {
    QFS_IDLE,
    QFS_WAITING_ERASE_FINISH,
    QFS_WAITING_FINISH,
} QFS_WriteStateMechine_t;

typedef enum {
    // 固化数据头状态
    QFS_WRITE_HEADER,
    // 固化队列数据体状态，QFS写位置状态机是每次固化数据头完成后又转到固化数据体状态
    QFS_WRITE_QUEUE_BODY,
} QFS_WriteLotateStateMechine_t;

// QFS写位置状态机
QFS_WriteLotateStateMechine_t qfs_wsm = QFS_WRITE_QUEUE_BODY;
// QFS状态机
QFS_WriteStateMechine_t qfs_sm = QFS_IDLE;
// QFS头部信息，也就是队列信息
QFSHeader_t header = {"QFS", 0, 0, 0, 0};

// QFS写入缓存队列,至少为一个W25Q512_SECTOR_SIZE的大小。
static CircularArrayQueue_t qfs_wqueue = {0};
#define QFS_WRITE_QUEUE_CAPACITY (W25Q512_SECTOR_SIZE * 3)
uint8_t qfs_wqueue_buf[QFS_WRITE_QUEUE_CAPACITY] = {0};
// QFS写入缓存队列数据到达这个阈值后才进入写数据到W25Q512的流程（也就是固化数据的流程）。
#define MIN_FLUSH_DATA_THRESHOLD W25Q512_SECTOR_SIZE
uint32_t flush_data2flash_threshold = MIN_FLUSH_DATA_THRESHOLD;
// 50代表50%
uint8_t flush_data2flash_threshold_persentage = 60;

// QFS写入缓存
uint8_t qfs_wbuffer[W25Q512_SECTOR_SIZE] = {0};

// 测试QFS写入缓存队列的性能所用到的变量
// 从写入缓存队列中读取到的字节数
uint32_t cache_queue_read_amount = 0;
// 从W25Q512中读取的字节数
uint32_t physical_storage_read_amount = 0;
// QFS总读取字节数
uint32_t total_read_amount = 0;
// QFS总写入字节数
uint32_t total_write_amount = 0;
// QFS平均读取速度KB/s
float average_read_speed = 0;
// QFS平均写入速度KB/s
float average_write_speed = 0;
// 多少个CPU_Tick,如果一个CPU Tick为1ms，那么就说speed_measure_time ms.
uint32_t speed_measure_time = 1000;
// 清空时清空了多少字节的数据
uint64_t make_empty_amount = 0;

/**
 * @brief 初始化队列文件系统
 *
 */
void CHIP_W25Q512_QFS_init()
{
    CHIP_W25Q512_init();

    // 初始化开始固化流程的阈值
    uint32_t tmp_threshold = flush_data2flash_threshold_persentage * 0.01f * QFS_WRITE_QUEUE_CAPACITY;
    if (tmp_threshold > MIN_FLUSH_DATA_THRESHOLD) {
        flush_data2flash_threshold = tmp_threshold;
    }

    // 创建QFS写入缓存队列
    c_arr_queue_create(&qfs_wqueue, qfs_wqueue_buf, sizeof(qfs_wqueue_buf));

    if (CHIP_W25Q512_QFS_isFormated()) {
        // 更新当前的状态到内存中
        QFSHeader_t _header;
        CHIP_W25Q512_read(LOGIC_SECTOR_OFFSET_OF_PHYSICAL_SECTOR_INDEX * W25Q512_SECTOR_SIZE, (uint8_t *)&_header, sizeof(QFSHeader_t));
        _header.font_sec_poped = 0;
        _header.rear_sec_used  = 0;
        // header = _header;
    } else {
        CHIP_W25Q512_QFS_format();
    }
}

/**
 * @brief 格式化Flash，也就是向Flash中写入存储信息,并且是阻塞执行的。
 *
 */
void CHIP_W25Q512_QFS_format()
{
    header.rear_sec_numb  = 0;
    header.rear_sec_used  = 0;
    header.font_sec_numb  = 0;
    header.font_sec_poped = 0;

    memset(qfs_wbuffer, 0, W25Q512_SECTOR_SIZE);
    memcpy(qfs_wbuffer, (uint8_t *)&header, sizeof(QFSHeader_t));
    w25q512_write_one_sector(LOGIC_SECTOR_OFFSET_OF_PHYSICAL_SECTOR_INDEX, qfs_wbuffer);
}

/**
 * @brief 检查是否格式化了
 *
 * @return int 没有格式化0，格式化了1
 */
int CHIP_W25Q512_QFS_isFormated()
{
    QFSHeader_t _header = {0};
    int status          = 0;
    CHIP_W25Q512_read(LOGIC_SECTOR_OFFSET_OF_PHYSICAL_SECTOR_INDEX * W25Q512_SECTOR_SIZE, (uint8_t *)&_header, sizeof(QFSHeader_t));
    if (memcmp("QFS", _header.flag, sizeof("QFS")) == 0) {
        status = 1;
    }
    return status;
}

/**
 * @brief 周期启动固化头部信息到物理存储。
 *
 */
static void CHIP_W25Q512_QFS_start_flush_header_period()
{
    static uint32_t cpu_tick = 0;
    // n ms 把队列的头部信息写到Flash中一次。
    if ((HDL_CPU_Time_GetTick() - cpu_tick) > 4000) {
        if (CHIP_W25Q512_QFS_start_flush_header() == 1) // 启动成功
        {
            cpu_tick = HDL_CPU_Time_GetTick();
        }
    }
}

void CHIP_W25Q512_QFS_handler()
{
    static int page_idx = 0; // Flash写入状态机
    switch (qfs_wsm) {
        // 如果当前状态位写数据体，那么获取缓存列表的第一个节点。
        case QFS_WRITE_QUEUE_BODY:
            switch (qfs_sm) {
                case QFS_IDLE:

                    // 检查队列是否包含一个W25Q512_SECTOR_SIZE的数据
                    if (c_arr_queue_size(&qfs_wqueue) >= flush_data2flash_threshold) {
                        if (!w25q512_is_busy()) {
                            c_arr_queue_out(&qfs_wqueue, qfs_wbuffer, W25Q512_SECTOR_SIZE);
                            w25q512_erase_one_sector_cmd(QFS_PHYSICAL_SECTOR_INDEX(header.rear_sec_numb));
                            qfs_sm = QFS_WAITING_ERASE_FINISH;
                        }
                    }
                    break;
                case QFS_WAITING_ERASE_FINISH:
                    if (!w25q512_is_busy()) {
                        qfs_sm = QFS_WAITING_FINISH;
                    }
                    break;

                case QFS_WAITING_FINISH:

                    if (page_idx < W25Q512_SECTOR_SIZE / W25Q512_PAGE_SIZE) {
                        if (!w25q512_is_busy()) {
                            uint32_t sector = QFS_PHYSICAL_SECTOR_INDEX(header.rear_sec_numb);
                            uint8_t *buf    = qfs_wbuffer;
                            w25q512_write_page_no_erase_no_wait(sector * W25Q512_SECTOR_SIZE + page_idx * W25Q512_PAGE_SIZE,
                                                                qfs_wbuffer + page_idx * W25Q512_PAGE_SIZE, W25Q512_PAGE_SIZE);
                            page_idx++;
                        }
                    } else
                    // 写入完成了
                    {
                        w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);

                        // QFS enqueue
                        header.rear_sec_numb = (header.rear_sec_numb + 1) % QFS_DATA_FILED_LOGIC_SECTOR_COUNT;

                        page_idx = 0;
                        qfs_sm   = QFS_IDLE;

                        // TODO
                        CHIP_W25Q512_QFS_start_flush_header_period();
                    }
                    break;
                default:
                    break;
            }
            break;
        // 如果当前状态为写数据头，那么切换当前node为数据头缓存节点。
        case QFS_WRITE_HEADER:

            switch (qfs_sm) {
                case QFS_IDLE:
                    if (!w25q512_is_busy()) {
                        w25q512_erase_one_sector_cmd(0);
                        qfs_sm = QFS_WAITING_ERASE_FINISH;
                    }
                    break;
                case QFS_WAITING_ERASE_FINISH:
                    if (!w25q512_is_busy()) {
                        qfs_sm = QFS_WAITING_FINISH;
                    }
                    break;

                case QFS_WAITING_FINISH:

                    if (page_idx < W25Q512_SECTOR_SIZE / W25Q512_PAGE_SIZE) {
                        if (!w25q512_is_busy()) {
                            uint32_t sector = 0;
                            uint8_t *buf    = qfs_wbuffer;
                            w25q512_write_page_no_erase_no_wait(sector * W25Q512_SECTOR_SIZE + page_idx * W25Q512_PAGE_SIZE, buf + page_idx * W25Q512_PAGE_SIZE, W25Q512_PAGE_SIZE);
                            page_idx++;
                        }
                    } else
                    // 写入完成了
                    {
                        w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);
                        page_idx = 0;
                        qfs_wsm  = QFS_WRITE_QUEUE_BODY;
                        qfs_sm   = QFS_IDLE;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

/**
 * @brief 向Flash中推送数据。
 *
 * @param buf 指向数据的指针。
 * @param len 数据的长度
 * @return uint32_t 成功返回实际写入字节数，失败返回0。
 */
uint32_t CHIP_W25Q512_QFS_push(uint8_t *buf, uint32_t len)
{
    int ret = 0;
    // 如果QFS满了，就不允许写入，即使写入缓存表能写
    if (!CHIP_W25Q512_QFS_is_full()) {
        ret = c_arr_queue_in(&qfs_wqueue, buf, len);
    }

    //@measure
    total_write_amount += ret;
    return ret;
}

/**
 * @brief 获取Flash队列首部扇区数据的地址。
 *
 * @return uint32_t 地址。
 */
uint32_t CHIP_W25Q512_QFS_get_font_address()
{
    return QFS_PHYSICAL_SECTOR_INDEX(header.font_sec_numb) * W25Q512_SECTOR_SIZE;
}

/**
 * @brief 获取Flash队列尾部扇区数据的地址。
 *
 * @return uint32_t 地址。
 */
uint32_t CHIP_W25Q512_QFS_get_rear_address()
{
    return QFS_PHYSICAL_SECTOR_INDEX(header.rear_sec_numb) * W25Q512_SECTOR_SIZE;
}

///*************************QFS队列相关部分*******************************/
/**
 * @brief 返回队列中的数据的扇区单元数。
 *
 * @return uint32_t 扇区单元数
 */
uint32_t CHIP_W25Q512_QFS_size()
{
    return (header.rear_sec_numb + QFS_DATA_FILED_LOGIC_SECTOR_COUNT - header.font_sec_numb) % QFS_DATA_FILED_LOGIC_SECTOR_COUNT;
}

uint8_t CHIP_W25Q512_QFS_is_empty()
{
    return header.rear_sec_numb == header.font_sec_numb;
}

uint8_t CHIP_W25Q512_QFS_is_full()
{
    return (header.rear_sec_numb + 1) % QFS_DATA_FILED_LOGIC_SECTOR_COUNT == header.font_sec_numb;
}

/**
 * @brief 清空QFS队列
 *
 * @return uint8_t
 */
uint8_t CHIP_W25Q512_QFS_make_empty()
{
    make_empty_amount += CHIP_W25Q512_QFS_byte_size();

    // 使得队列为空
    header.font_sec_numb = header.rear_sec_numb;

    // 清空已经出队的字节数，避免对
    // CHIP_W25Q512_QFS_asyn_read,CHIP_W25Q512_QFS_asyn_pop,
    // CHIP_W25Q512_QFS_byte_size,CHIP_W25Q512_QFS_asyn_readable_byte_size
    // 和 CHIP_W25Q512_QFS_pop
    // 方法造成影响
    header.font_sec_poped = 0;

    // 因为QFS状态机只是在改变rear_sec_numb，
    // 所以无论现在处于何种状态修改font_sec_numb都不影响写入过程
    return 0;
}

/**
 * @brief Flash队列任意大小数据出队，这个方法只会在QFS中有已经写入的数据是才能读取数据。
 *
 * @param buf 存放出队数据的指针，如果为NULL表示只出队不读取。buf指向内存块最小大小为W25Q512_SECTOR_SIZE。
 * @param pop_len 希望出队的字节数。
 * @return uint32_t 实际出队的字节数，也就是0表示队列没有数据，>0表示实际出队的字节数。
 */
uint32_t CHIP_W25Q512_QFS_pop(uint8_t *buf, uint32_t pop_len)
{
    uint32_t ret = 0;
    uint32_t res = 0;

    while (pop_len > 0 && !CHIP_W25Q512_QFS_is_empty()) {
        // 当前font所在扇区剩数据大小
        res = W25Q512_SECTOR_SIZE - header.font_sec_poped;
        // 如果当前font所在扇区剩余数据大小大于等于pop_len的数据，那么直接读取
        if (res >= pop_len) {
            if (buf != NULL) {
                w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);
                CHIP_W25Q512_read(CHIP_W25Q512_QFS_get_font_address() + header.font_sec_poped, buf, pop_len);
            }

            header.font_sec_poped += pop_len;
            // 如果当前font所在扇区已经读取了W25Q512_SECTOR_SIZE字节的数据，就相当于一次扇区pop queue
            // 这里会保证font_sec_poped只会增加到W25Q512_SECTOR_SIZE
            if (header.font_sec_poped == W25Q512_SECTOR_SIZE) {
                header.font_sec_poped = 0;
                header.font_sec_numb  = (header.font_sec_numb + 1) % QFS_DATA_FILED_LOGIC_SECTOR_COUNT;
            }

            ret += pop_len;
            pop_len = 0;
        } else {
            if (buf != NULL) {
                w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);
                CHIP_W25Q512_read(CHIP_W25Q512_QFS_get_font_address() + header.font_sec_poped, buf, res);
            }

            header.font_sec_numb  = (header.font_sec_numb + 1) % QFS_DATA_FILED_LOGIC_SECTOR_COUNT;
            header.font_sec_poped = 0;

            buf += res;
            pop_len -= res;
            ret += res;
        }
    }

    return ret;
}

/**
 * @brief 这个方法同CHIP_W25Q512_QFS_pop，不同之处在于这个方法需要用户自行判断当前是否可以读取数据。
 * 这个方法只会在QFS中有已经写入的数据是才能读取数据
 * @warning 读写始终在同一个线程中。其次是如果写的特别快，而读取周期慢，那么有可能始终芯片都处于忙状态导致无法读取。
 *
 * @param buf 存放出队数据的指针，如果为NULL表示只出队不读取。buf指向内存块最小大小为W25Q512_SECTOR_SIZE。
 * @param pop_len 希望出队的字节数。
 * @return uint32_t 实际出队的字节数，也就是0表示队列没有数据，>0表示实际出队的字节数。
 */
uint32_t CHIP_W25Q512_QFS_asyn_pop(uint8_t *buf, uint32_t pop_len)
{
    uint32_t ret = 0;
    uint32_t res = 0;

    while (pop_len > 0 && !CHIP_W25Q512_QFS_is_empty()) {
        // 当前font所在扇区剩数据大小
        res = W25Q512_SECTOR_SIZE - header.font_sec_poped;
        // 如果当前font所在扇区剩余数据大小大于等于pop_len的数据，那么直接读取
        if (res >= pop_len) {
            if (buf != NULL) {
                w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);
                CHIP_W25Q512_read(CHIP_W25Q512_QFS_get_font_address() + header.font_sec_poped, buf, pop_len);
            }

            header.font_sec_poped += pop_len;
            // 如果当前font所在扇区已经读取了W25Q512_SECTOR_SIZE字节的数据，就相当于一次扇区pop queue
            // 这里会保证font_sec_poped只会增加到W25Q512_SECTOR_SIZE
            if (header.font_sec_poped == W25Q512_SECTOR_SIZE) {
                header.font_sec_poped = 0;
                header.font_sec_numb  = (header.font_sec_numb + 1) % QFS_DATA_FILED_LOGIC_SECTOR_COUNT;
            }

            ret += pop_len;
            pop_len = 0;
        } else {
            if (buf != NULL) {
                w25q512_wait_busy(W25Q512_TIMEOUT_DEFAULT_VALUE);
                CHIP_W25Q512_read(CHIP_W25Q512_QFS_get_font_address() + header.font_sec_poped, buf, res);
            }

            header.font_sec_numb  = (header.font_sec_numb + 1) % QFS_DATA_FILED_LOGIC_SECTOR_COUNT;
            header.font_sec_poped = 0;

            buf += res;
            pop_len -= res;
            ret += res;
        }
    }

    return ret;
}

/**
 * @brief 从QFS中读取数据，这个方法在QFS状态为IDLE且QFS为空时会从缓存队列直接取数据。
 * 其次这个方法保证只会在QFS状态为IDLE时读取数据，这就造成了可能写入缓存已经有数据，但
 * 是QFS状态不为IDLE，无法读取数据。也就是这个方法读取不到数据不代表真的没有数据，而可
 * 能是不可读。
 * 如果当QFS的状态不为IDLE且QFS不为空时一定要读取数据那么就需要阻塞等待W25Q512忙状态
 * 结束才能读取。请使用CHIP_W25Q512_QFS_asyn_pop方法，使用前。
 *
 * 注意：这个方法实际上是read and pop，被读取的数据相当于出队了。
 * @param buf
 * @param len 希望出队的字节数。
 * @return uint32_t 实际出队的字节数，也就是0表示队列没有数据，>0表示实际出队的字节数。
 */
uint32_t CHIP_W25Q512_QFS_asyn_read(uint8_t *buf, uint32_t len)
{
    uint32_t ret = 0;
    uint32_t tmp = 0;
    // 两个结束条件
    //  1. 读到了len个数据
    //  2. 没有允许读取的数据了
    while (len > 0) {
        switch (qfs_wsm) {
            case QFS_WRITE_QUEUE_BODY:
                // CHIP_W25Q512_QFS_asyn_readable()
                if (qfs_sm == QFS_IDLE) {
                    // 首先是要读取QFS中存放到物理存储的数据
                    if (CHIP_W25Q512_QFS_is_empty()) {
                        tmp = c_arr_queue_out(&qfs_wqueue, buf, len);
                        len -= tmp;
                        buf += tmp;
                        ret += tmp;
                        cache_queue_read_amount += tmp;
                        if (c_arr_queue_is_empty(&qfs_wqueue)) {
                            //@measure
                            total_read_amount += ret;
                            return ret;
                        }
                    } else {
                        tmp = CHIP_W25Q512_QFS_asyn_pop(buf, len);
                        len -= tmp;
                        buf += tmp;
                        ret += tmp;
                        physical_storage_read_amount += tmp;
                    }
                } else {
                    //@measure
                    total_read_amount += ret;
                    return ret;
                }
                break;
            case QFS_WRITE_HEADER:
                if (CHIP_W25Q512_QFS_is_empty()) {
                    tmp = c_arr_queue_out(&qfs_wqueue, buf, len);
                    len -= tmp;
                    buf += tmp;
                    ret += tmp;
                    cache_queue_read_amount += tmp;
                    if (c_arr_queue_is_empty(&qfs_wqueue)) {
                        //@measure
                        total_read_amount += ret;
                        return ret;
                    }
                } else {
                    //@measure
                    total_read_amount += ret;
                    return ret;
                }
                break;
            default:
                break;
        }
    }
    //@measure
    total_read_amount += ret;
    return ret;
}

/**
 * @brief 检查QFS是否可以读取。
 *
 * @return uint8_t 可以读取1, 不可读0
 */
uint8_t CHIP_W25Q512_QFS_asyn_readable()
{
    return qfs_sm == QFS_IDLE;
}

/**
 * @brief 返回QFS队列中可读取的字节数。但是返回0时一定代表不可以读取，有可能时芯片忙。
 *
 * @return uint32_t 0没有数据或者芯片忙碌，>0队列中当前可以读取的字节数。
 */
uint32_t CHIP_W25Q512_QFS_asyn_readable_byte_size()
{
    uint32_t ret = 0;
    if (CHIP_W25Q512_QFS_asyn_readable()) {
        ret = CHIP_W25Q512_QFS_byte_size();
    }
    return ret;
}

/**
 * @brief 启动固化QFS头部信息到Flash的过程，启动后可能QFS写入状态机qfs_sm
 * 还在处于非QFS_IDLE的状态，这会导致启动失败。
 *
 * @note 这个函数不支持重入，必须和CHIP_W25Q512_QFS_handler在一个线程。
 * @return int 1 已经直接启动QFS头部信息固化过程，0 启动失败。
 */
static int CHIP_W25Q512_QFS_start_flush_header()
{
    int ret = 0;
    if (qfs_sm == QFS_IDLE && qfs_wsm == QFS_WRITE_QUEUE_BODY) {
        memset(qfs_wbuffer, 0, W25Q512_SECTOR_SIZE);
        memcpy(qfs_wbuffer, (uint8_t *)&header, sizeof(QFSHeader_t));
        qfs_wsm = QFS_WRITE_HEADER;
        ret     = 1;
    }
    return ret;
}

// 下面的方法没啥用

/**
 * @brief 从队列尾部读取一个单元的数据。
 *
 * @param buf 存放数据的内存的指针。
 * @return uint32_t 实际读取的单元（扇区）数，也就是0表示队列没有数据，1表示出队成功。
 */
uint32_t CHIP_W25Q512_QFS_font(uint8_t *buf)
{
    uint32_t ret = 0;
    if (!CHIP_W25Q512_QFS_is_empty()) {
        if (buf != NULL) {
            CHIP_W25Q512_read(CHIP_W25Q512_QFS_get_font_address(), buf, W25Q512_SECTOR_SIZE);
        }
        ret = 1;
    }
    return ret;
}

/**
 * @brief 返回QFS队列中的数据的字节数。这个字节数是已经存放到FLASH的数据减去pop掉的字节数。
 *
 * @return uint32_t 字节数
 */
uint32_t CHIP_W25Q512_QFS_byte_size()
{
    return CHIP_W25Q512_QFS_size() * W25Q512_SECTOR_SIZE - header.font_sec_poped;
}

/**
 * @brief QFS状态机是否处于空闲状态。
 *
 * @return uint8_t 1处于空闲状态，0处于忙碌状态
 */
uint8_t CHIP_W25Q512_QFS_is_idle()
{
    return (uint8_t)(qfs_sm == QFS_IDLE);
}

//

/**
 * @brief W25Q512读取一个扇区的数据。
 *
 * @param sec_idx 扇区物理索引。
 * @param buf 确保buf有足够的空间存放一个扇区的数据。
 * @return int32_t 1成功，0失败。
 */
int32_t CHIP_W25Q512_read_one_sector(uint32_t sec_idx, uint8_t *buf)
{
    int32_t ret = 0;
    if (buf != NULL) {
        CHIP_W25Q512_read(sec_idx * W25Q512_SECTOR_SIZE, buf, W25Q512_SECTOR_SIZE);
        ret = 1;
    }
    return ret;
}