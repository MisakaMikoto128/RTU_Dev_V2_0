#include "APP_Sampler.h"
#include "CHIP_W25Q512_QueueFileSystem.h"
#include "HDL_RTC.h"
/**
 * @brief 将字节流解析为一个采样点数据包。
 *
 * @param data 16字节的数据指针。
 * @param pvar 存放解析结果的数据包。
 * @return uint8_t 0解码成功，>0校验和错误，但是结果任然对应输入的数据。
 */
uint8_t RTU_Sampling_Var_decoder(const uint8_t *data, RTU_Sampling_Var_t *pvar)
{
    *pvar = *((RTU_Sampling_Var_t *)data);
    return RTU_Sampling_Var_checksum(pvar) == pvar->checksum;
}

/**
 * @brief 从采样点中获取本地日期时间对象。
 *
 * @param pvar
 * @param pdatetime
 */
void RTU_Sampling_Var_get_calibration_time_local(RTU_Sampling_Var_t *pvar, mtime_t *pdatetime)
{
    uint64_t datetime_ms  = *((uint64_t *)(pvar->data));
    uint32_t datetime_sec = datetime_ms / 1000;
    uint32_t ms           = datetime_ms - datetime_sec * 1000;
    utc_sec_2_mytime(datetime_sec + 8UL * 60UL * 60UL, pdatetime);
    pdatetime->wSub = HDL_RTC_mSec2Subsec(ms);
}

extern APP_Sensor_t attitude_sensor;
extern APP_Sensor_t inclination_angle_sensor;

static struct sc_list _gEnabledSensorList = {NULL, NULL};
static struct sc_list *gEnabledSensorList = &_gEnabledSensorList;

bool APP_Sampler_is_sensor_enabled(uint8_t sensor_code);

/**
 * @brief 初始化采样器。
 *
 */
void APP_Sampler_init()
{
    sc_list_init(gEnabledSensorList);
}

/**
 * @brief 通过传感器类型码找到传感器对象。
 *
 * @param sensor_code 传感器类型码。
 * @return APP_Sensor_t* 成功返回对应传感器对象的指针，失败返回NULL
 */
APP_Sensor_t *APP_Sampler_find_sensor_by_sensor_code(uint8_t sensor_code)
{
    APP_Sensor_t *sensor = NULL;
    switch (sensor_code) {
        case SENSOR_CODE_WIND_SPEED_AND_DIRECTION:

            break;

        case SENSOR_CODE_SOLAR_RADIATION:

            break;
        case SENSOR_CODE_ANCHOR_CABLE_GAUGE:

            break;
        case SENSOR_CODE_VOLTAGE_CURRENT:

            break;
        case SENSOR_CODE_DISPLACEMENT:

            break;
        case SENSOR_CODE_ATTITUDE_SENSOR:
            sensor = &attitude_sensor;
            break;
        case SENSOR_CODE_INCLINATION_ANGLE_SENSOR:
            sensor = &inclination_angle_sensor;
            break;
        case SENSOR_CODE_PULL_PRESSURE:

            break;
        case SENSOR_CODE_TEMP_HUMI:

            break;
        default:
            sensor = NULL;
            break;
    }
    return sensor;
}

/**
 * @brief 从任务列表中移除所有传感器任务。
 *
 * @param sensor_code 传感器类型码。
 */
void APP_Sampler_disable_all_sensor_task()
{
    struct sc_list *it    = NULL;
    APP_Sensor_t *_sensor = NULL;
    struct sc_list *item, *tmp;

    sc_list_foreach_safe(gEnabledSensorList, tmp, it)
    {
        _sensor = sc_list_entry(it, APP_Sensor_t, next);
        scheduler_unregister(&_sensor->task);
        sc_list_del(gEnabledSensorList, &_sensor->next);
    }
}

/**
 * @brief 更换当前的传感器。这个函数假设当前只有一个传感器时使用。
 *
 * @param sensor_code
 */
void APP_Sampler_change_current_enabled_sensor(uint8_t sensor_code)
{
    APP_Sensor_t *sensor = NULL;
    sensor               = APP_Sampler_find_sensor_by_sensor_code(sensor_code);
    if (sensor != NULL) {
        APP_Sampler_disable_all_sensor_task();
        APP_Sampler_enable_sensor(sensor_code);
    }
}

/**
 * @brief 使能指定传感器。
 *
 * @param sensor_code
 */
void APP_Sampler_enable_sensor(uint8_t sensor_code)
{
    APP_Sensor_t *sensor = NULL;

    if (!APP_Sampler_is_sensor_enabled(sensor_code)) {
        sensor = APP_Sampler_find_sensor_by_sensor_code(sensor_code);
        if (sensor != NULL) {
            scheduler_register(&sensor->task);
            sc_list_init(&sensor->next);
            sc_list_add_tail(gEnabledSensorList, &sensor->next);
        }
    }
}

/**
 * @brief 设置传感器的采样频率。
 *
 * @param sensor_code
 * @param freq 1-1000Hz
 */
void APP_Sampler_set_sensor_freq(uint8_t sensor_code, int freq)
{
    APP_Sensor_t *sensor = NULL;

    if (sensor != NULL) {
        sensor = APP_Sampler_find_sensor_by_sensor_code(sensor_code);
        scheduler_set_freq(&sensor->task, freq);
    }
}

/**
 * @brief 传感器是否已经使能。
 *
 * @param sensor_code
 * @return true 使能。
 * @return false 未使能。
 */
bool APP_Sampler_is_sensor_enabled(uint8_t sensor_code)
{
    bool ret                 = false;
    APP_Sensor_t *aim_sensor = NULL; // User object
    APP_Sensor_t *sensor     = NULL; // User object

    struct sc_list *it; // Iterator
    aim_sensor = APP_Sampler_find_sensor_by_sensor_code(sensor_code);
    if (aim_sensor != NULL) {
        sc_list_foreach(gEnabledSensorList, it)
        {
            sensor = sc_list_entry(it, APP_Sensor_t, next);
            if (sensor == aim_sensor) {
                ret = true;
                break;
            }
        }
    }
    return ret;
}

/**
 * @brief 采样器轮询方法。
 *
 */
void APP_Sampler_handler()
{
    static APP_Sensor_t *sensor = NULL; // User object
    static struct sc_list *it;          // Iterator
    sc_list_foreach(gEnabledSensorList, it)
    {
        sensor = sc_list_entry(it, APP_Sensor_t, next);
        sensor->handler();
    }
}

/**
 * @brief 读取一个采样点出来,读取出来后数据就不在内部的QFS队列中了。
 *
 * @param pvar
 * @return uint8_t 1读取成功，0读取失败。
 */
uint8_t APP_Sampler_read(RTU_Sampling_Var_t *pvar)
{
    uint8_t ret  = 0;
    uint32_t len = 0;
    len          = CHIP_W25Q512_QFS_asyn_read((uint8_t *)pvar, sizeof(RTU_Sampling_Var_t));
    ret          = len > 0;
    return ret;
}