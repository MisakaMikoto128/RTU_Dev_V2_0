#include "CHIP_SHT30.h"
#include "HDL_CPU_Time.h"

void CHIP_SHT30_Write(uint16_t CMD)
{
    IIC_Start();
    IIC_Send_Byte(SHT30_ADD_WRITE);
    IIC_Wait_Ack();
    IIC_Send_Byte((CMD >> 8) & 0xff);
    IIC_Wait_Ack();
    IIC_Send_Byte(CMD & 0xff);
    IIC_Wait_Ack();
    IIC_Stop();
    HDL_CPU_Time_DelayMs(15);
}

void CHIP_SHT30_Init()
{
    IIC_Init();
    CHIP_SHT30_Write(CMD_MEAS_CLOCKSTR_H);
    HDL_CPU_Time_DelayMs(10);
}

void CHIP_SHT30_Read(sht30Var_t *pSht30Var)
{
    uint16_t buff[6];
    uint16_t temp, humi;
    CHIP_SHT30_Write(CMD_MEAS_CLOCKSTR_H);
    IIC_Start();
    IIC_Send_Byte(SHT30_ADD_READ);
    if (IIC_Wait_Ack() == 0) {
        buff[0] = IIC_Read_Byte(1); // 温度高八位
        buff[1] = IIC_Read_Byte(1); // 温度第八位
        buff[2] = IIC_Read_Byte(1);
        buff[3] = IIC_Read_Byte(1); // 湿度高八位
        buff[4] = IIC_Read_Byte(1); // 湿度低八位
        buff[5] = IIC_Read_Byte(0);
        IIC_Stop();
    }
    temp = (buff[0] << 8 | buff[1]);
    humi = (buff[3] << 8 | buff[4]);

    // 原始数据 -> 实际数据  ×10  (234 -> 23.4)
    pSht30Var->temp = 1750 * temp / 65535 - 450;
    pSht30Var->humi = 1000 * humi / 65535;
}

/*传输结果标志位：其逻辑图见《光伏RTU Modbus RTU传输协议实现》中所示。
因为一个总线一次只有一个传输，所以只有一个标志位。
该标志位在每次重新发送命令时清除为0，每次收到正确应答化为1，传输错误大于1：传输超时为2，帧错误为3。
*/
typedef enum {
    SHT30_TRANS_RES_NONE          = 0,
    SHT30_TRANS_REV_ACK           = 1,
    SHT30_TRANS_ERR_TRANS_TIMEOUT = 2,
} SHT30TranResult_t;

typedef struct tagSHT30Tran_t {
    /*传输结果标志位：其逻辑图见《光伏RTU Modbus RTU传输协议实现》中所示。
    因为一个总线一次只有一个传输，所以只有一个标志位。
    该标志位在每次重新发送命令时清除为0，每次收到正确应答化为1，传输错误大于1：传输超时为2，帧错误为3。
    即这个传输的结果会保留到处理完成接收到的数据。
    */
    SHT30TranResult_t transResult;

    /*传输步骤状态标志位：0表示该modbus总线当前空闲（本次传输的的处理完成），1表示正处于一个传输周期，2表示本次传输需要处理。
    这是一个关键变量，尤其是可能一部分在中断中执行，一部分在用户线程中执行，建议更换为原子变量。
    传输正常完成需要处理数据（正常数据和错误报告），传输超时需要做超时处理。
    该标志位在每次传输启动时置位，每次超时或者收到回复时变为待处理。通过调用读取方法清除或者手动清除。
    如果上次的传输没有及时处理，没有去清除标志位，那么就无法启动下一次传输。*/
    uint8_t transStageState;
    /*一次传输开始时刻，在每次发送命令时更新。例如发送一个读取命令到成功接收到对应消息算作一次传输。*/
    uint32_t tickstart;
    sht30Var_t var;
} SHT30Tran_t;

#define SHT30_TRANS_STATE_IDLE        0
#define SHT30_TRANS_STATE_TRANSMITING 1
// 等待处理意味着传输到了需要处理数据或者错误的阶段，这些错误和数据称为事件。
#define SHT30_TRANS_STATE_WAITING_DISPOSE 2
#define SHT30_SET_TRANS_STATE(state)      (hsht32Tran->transStageState = (state))
#define SHT30_RESET_TRANS_STATE()         (hsht32Tran->transStageState = SHT30_TRANS_STATE_IDLE)
#define SHT30_TRANS_TIMEOUT               200 /* 接收命令超时时间, 单位ms */
#define SHT30_CONVERT_PERIOD              16  /* SHT30 IIC 发送命令后时间到了可以读取数据的时间间隔，需要大于15ms */

SHT30Tran_t sht32Tran = {
    .transStageState = SHT30_TRANS_STATE_IDLE,
};
SHT30Tran_t *hsht32Tran = &sht32Tran;

/**
 * @brief 向SHT32发送一个读取数据的请求，如果先前发送的请求还在传输中
 * 或者上次传输的结果（也就是产生的事件还没有处理完成）则本次请求失败。
 *
 * @return uint8_t 1请求数据成功，0请求失败。
 */
uint8_t CHIP_SHT30_request()
{
    uint8_t status = 0;
    // 处于空闲状态
    if (hsht32Tran->transStageState == SHT30_TRANS_STATE_IDLE) {

        /* 发送命令 */
        {
            IIC_Start();
            IIC_Send_Byte(SHT30_ADD_WRITE);
            IIC_Wait_Ack();
            IIC_Send_Byte((CMD_MEAS_CLOCKSTR_H >> 8) & 0xff);
            IIC_Wait_Ack();
            IIC_Send_Byte(CMD_MEAS_CLOCKSTR_H & 0xff);
            IIC_Wait_Ack();
            IIC_Stop();
        }
        hsht32Tran->transResult     = SHT30_TRANS_RES_NONE;
        hsht32Tran->transStageState = SHT30_TRANS_STATE_TRANSMITING;
        hsht32Tran->tickstart       = HDL_CPU_Time_GetTick(); /* 记录命令发送的时刻 */

        status = 1;
    }

    return status;
}

/**
 * @brief 查询是否有事件需要处理。事件包括收到数据、超时。
 *
 * @return uint8_t 0表示没有事件需要处理，1表示需要处理收到的回复，大于1表示出现超时错误。
 */
uint8_t CHIP_SHT30_query_event()
{
    uint8_t status = 0;
    if (hsht32Tran->transStageState == SHT30_TRANS_STATE_WAITING_DISPOSE) {
        status = hsht32Tran->transResult;
    }
    return status;
}

/**
 * @brief 查询是否有事件需要处理。事件包括收到数据、超时。与CHIP_SHT30_query_event不同之处在于
 * 这个方法在查询过程中发现超时错误会在内部直接处理掉，而不会留给用户去处理。
 *
 * @return uint8_t 0表示没有事件需要处理，1表示需要处理收到的回复.。
 */
uint8_t CHIP_SHT30_query_result()
{
    uint8_t status = 0;
    if (hsht32Tran->transStageState == SHT30_TRANS_STATE_WAITING_DISPOSE) {
        // 在内部就处理掉一些列错误事件，例如超时，这样就不需要用户操心超时的问题了。
        {
            switch (hsht32Tran->transResult) {
                case SHT30_TRANS_RES_NONE:
                    break;
                case SHT30_TRANS_REV_ACK:
                    status = 1;
                    break;
                case SHT30_TRANS_ERR_TRANS_TIMEOUT:
                    SHT30_RESET_TRANS_STATE();
                    break;
                default:
                    break;
            }
        }
    }
    return status;
}

/**
 * @brief 清除所有事件,完成传输最后一个步骤(等待处理)。在每次在每次CHIP_SHT30_query_result()
 * 查询到有事件后处理完事件执行。不执行的话下一次命令无法发送。
 * 负责完成传输的重置和传输超时处理。
 *
 */
void CHIP_SHT30_clear_all_event()
{
    SHT30_RESET_TRANS_STATE();
}

/**
 * @brief 用于CHIP_SHT30_query_result或者CHIP_SHT30_query_event再查询到有收到数据之后读取数据到用户程序的方法。
 *
 * @param pSht30Var 存放SHT30收到结果的对象的指针。
 */
void CHIP_SHT30_get_result(sht30Var_t *pSht30Var)
{
    *pSht30Var = sht32Tran.var;
}

/**
 * @brief
 *
 */
void CHIP_SHT30_handler()
{
    switch (hsht32Tran->transStageState) {
        case SHT30_TRANS_STATE_IDLE:
            break;

            // 处在传输中等待响应。
        case SHT30_TRANS_STATE_TRANSMITING: {
            if ((HDL_CPU_Time_GetTick() - hsht32Tran->tickstart) > SHT30_TRANS_TIMEOUT) {
                hsht32Tran->transResult = SHT30_TRANS_ERR_TRANS_TIMEOUT; /* 通信超时了 */
                // 超时，传输结束，转变为等待处理传输
                SHT30_SET_TRANS_STATE(SHT30_TRANS_STATE_WAITING_DISPOSE);
            }

            // TODO:pull msg
            // SHT30 IIC 发送命令后时间到了可以读取数据的时刻
            if ((HDL_CPU_Time_GetTick() - hsht32Tran->tickstart) > SHT30_CONVERT_PERIOD) {
                uint16_t buff[6];
                uint16_t temp, humi;

                IIC_Start();
                IIC_Send_Byte(SHT30_ADD_READ);
                if (IIC_Wait_Ack() == 0) {
                    buff[0] = IIC_Read_Byte(1); // 温度高八位
                    buff[1] = IIC_Read_Byte(1); // 温度第八位
                    buff[2] = IIC_Read_Byte(1);
                    buff[3] = IIC_Read_Byte(1); // 湿度高八位
                    buff[4] = IIC_Read_Byte(1); // 湿度低八位
                    buff[5] = IIC_Read_Byte(0);
                    IIC_Stop();
                }
                temp = (buff[0] << 8 | buff[1]);
                humi = (buff[3] << 8 | buff[4]);

                // 原始数据 -> 实际数据  ×10  (234 -> 23.4)
                hsht32Tran->var.temp    = 1750 * temp / 65535U - 450;
                hsht32Tran->var.humi    = 1000 * humi / 65535U;
                hsht32Tran->transResult = SHT30_TRANS_REV_ACK;
                // 正确接收数据，传输结束，转变为等待处理传输
                SHT30_SET_TRANS_STATE(SHT30_TRANS_STATE_WAITING_DISPOSE);
            }
        }

        break;
        case SHT30_TRANS_STATE_WAITING_DISPOSE:
            break;
        default:
            break;
    }
}
