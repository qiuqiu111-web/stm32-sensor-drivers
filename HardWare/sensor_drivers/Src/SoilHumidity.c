#include "main.h"
#include "func_config.h"
#include "SoilHumidity.h"

// 启动ADC转换
static eSoilHumidity_error_code _soil_humidity_start_conv(SoilHumidity_Handle *handle) {
    if (HAL_ADC_Start(handle->hadc) != HAL_OK) {
        return SOIL_HUMIDITY_ERROR_ADC;
    }
    return SOIL_HUMIDITY_OK;
}

// 读取ADC转换结果
static eSoilHumidity_error_code _soil_humidity_read_conv(SoilHumidity_Handle *handle) {
    if (HAL_ADC_PollForConversion(handle->hadc, 100) == HAL_OK) {
        handle->raw_data = HAL_ADC_GetValue(handle->hadc);
        HAL_ADC_Stop(handle->hadc);
        return SOIL_HUMIDITY_OK;
    } else {
        HAL_ADC_Stop(handle->hadc);
        return SOIL_HUMIDITY_ERROR_TIMEOUT;
    }
}

// 计算土壤湿度百分比
static void _soil_humidity_calc(SoilHumidity_Handle *handle) {
    if (handle->calibrated) {
        // 典型的LM393土壤湿度传感器：干燥时ADC值高，湿润时ADC值低
        if (handle->raw_data >= handle->calib_dry) {
            handle->humidity = 0.0f;
        } else if (handle->raw_data <= handle->calib_wet) {
            handle->humidity = 100.0f;
        } else {
            handle->humidity = (float)(handle->calib_dry - handle->raw_data) /
                               (float)(handle->calib_dry - handle->calib_wet) * 100.0f;
        }
    } else {
        // 未校准时直接输出原始ADC值的百分比
        handle->humidity = (float)handle->raw_data / 4095.0f * 100.0f;
    }
    handle->data_flag = 1;
}

// 初始化
int soil_humidity_init(SoilHumidity_Handle *handle) {
    if (!handle || !handle->hadc) return -1;

    handle->status = SOIL_HUMIDITY_READY;
    handle->raw_data = 0;
    handle->data_flag = 0;
    handle->retry_count = 0;
    handle->start_time = 0;
    handle->humidity = 0.0f;
    return 0;
}

// 设置校准值
int soil_humidity_set_calib(SoilHumidity_Handle *handle, uint16_t dry, uint16_t wet) {
    if (!handle) return -1;

    handle->calib_dry = dry;
    handle->calib_wet = wet;
    handle->calibrated = 1;
    return 0;
}

// 获取已准备好的数据
int soil_humidity_get(SoilHumidity_Handle *handle, float *humidity) {
    if (!handle || !humidity) return -1;
    if (handle->data_flag != 1) return -2;

    *humidity = handle->humidity;
    handle->data_flag = 0;
    return 0;
}

// 状态机驱动
void soil_humidity_run(SoilHumidity_Handle *handle) {
    if (!handle) return;

    // 重试次数达到上限，重置为初始状态
    if (handle->retry_count >= SOIL_HUMIDITY_MAX_RETRY_COUNT) {
        handle->status = SOIL_HUMIDITY_READY;
        handle->retry_count = 0;
        handle->data_flag = 0;
        return;
    }

    switch (handle->status) {

        case SOIL_HUMIDITY_READY:
            // 准备开始测量，进入校准（启动ADC转换）
            if (_soil_humidity_start_conv(handle) == SOIL_HUMIDITY_OK) {
                handle->status = SOIL_HUMIDITY_CALIBRATE;
                handle->start_time = GET_TIME_MS();
                handle->retry_count = 0;
            } else {
                handle->retry_count++;
            }
            break;

        case SOIL_HUMIDITY_CALIBRATE:
            // 已启动ADC转换，等待转换完成并读取数据
            if (_soil_humidity_read_conv(handle) == SOIL_HUMIDITY_OK) {
                _soil_humidity_calc(handle);
                handle->status = SOIL_HUMIDITY_COMPLETE;
                handle->retry_count = 0;
            } else {
                handle->retry_count++;
            }
            break;

        case SOIL_HUMIDITY_COMPLETE:
            // 数据就绪，等待用户读取
            if (handle->data_flag == 0) {
                // 数据已被读取，开始新一轮测量
                handle->status = SOIL_HUMIDITY_READY;
            }
            break;

        default:
            handle->status = SOIL_HUMIDITY_READY;
            break;
    }
}

int soil_humidity_ready(SoilHumidity_Handle *handle) {
    if (!handle) return -1;
    return (handle->data_flag == 1) ? 0 : -2; // 0: ready, -2: no data
}