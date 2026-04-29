#include "main.h"
#include "GY30.h"

// 发送命令到GY30
static eGY30_error_code _gy30_send_command(GY30_Handle *handle, uint8_t cmd) {
    HAL_StatusTypeDef hal_status = I2C_MASTER_TRANSMIT(handle->i2c, &cmd, 1);
    if (hal_status != HAL_OK) {
        return GY30_ERROR_I2C;
    }
    return GY30_OK;
}

// 读取测量数据（2字节）
static eGY30_error_code _gy30_read_data(GY30_Handle *handle) {
    uint8_t buffer[2] = {0};
    HAL_StatusTypeDef hal_status = I2C_MASTER_RECEIVE(handle->i2c, buffer, 2);
    if (hal_status != HAL_OK) {
        return GY30_ERROR_I2C;
    }
    // 数据为大端序：第一个字节是高8位
    handle->raw_data = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
    return GY30_OK;
}

// 处理原始数据，转换为照度值（lx）
static void _gy30_process_data(GY30_Handle *handle) {
    // 根据测量模式进行转换
    // 高分辨率模式：照度 = 原始值 / 1.2
    // 低分辨率模式：照度 = 原始值 / 1.2 * 2（但实际低分辨率模式分辨率不同，简化处理）
    if (handle->mode == GY30_CONT_LR_MODE || handle->mode == GY30_ONE_LR_MODE) {
        handle->illuminance = (float)handle->raw_data / 1.2f * 2.0f; // 低分辨率模式乘以2
    } 
    else {
        handle->illuminance = (float)handle->raw_data / 1.2f; // 高分辨率模式
    }
    handle->data_flag = 1; // 数据准备好
}

// 检查数据是否在合理范围内
static eGY30_error_code _gy30_check_data(GY30_Handle *handle) {
    // BH1750测量范围：0 ~ 65535 lx（理论上）
    // 照度值合理性检查（可根据实际应用调整）
    if (handle->illuminance < 0.0f || handle->illuminance > 100000.0f) {
        handle->data_flag = 0;
        return GY30_ERROR_DATA;
    }
    return GY30_OK;
}

// GY30初始化函数
int gy30_init(GY30_Handle *handle) {
    if (!handle) return -1;

    // 发送上电命令
    eGY30_error_code err = _gy30_send_command(handle, GY30_POWER_ON);
    if (err != GY30_OK) {
        return -2; // I2C通信失败
    }

    // 设置默认测量模式
    err = _gy30_send_command(handle, GY30_DEFAULT_MODE);
    if (err != GY30_OK) {
        return -3;
    }

    handle->mode = GY30_DEFAULT_MODE;
    handle->status = GY30_START;
    handle->start_time = GET_TIME_MS(); // 记录开始时间
    handle->data_flag = 0;
    handle->retry_count = 0; // 重置重试计数
    return 0; // 初始化成功
}

// 设置测量模式
int gy30_set_mode(GY30_Handle *handle, uint8_t mode) {
    if (!handle) return -1;

    // 发送测量模式命令
    eGY30_error_code err = _gy30_send_command(handle, mode);
    if (err != GY30_OK) {
        // 发送失败，重置状态
        handle->status = GY30_EMPTY;
        return -2; // I2C通信失败
    }

    handle->mode = mode;
    handle->status = GY30_START;
    handle->start_time = GET_TIME_MS(); // 记录开始时间
    handle->data_flag = 0;
    handle->retry_count = 0; // 重置重试计数

    return 0; // 设置成功
}

// 获取已准备好的数据
int gy30_get(GY30_Handle *handle, float *illuminance) {
    if (!handle || !illuminance) return -1;
    if (handle->data_flag != 1) return -2; // 无数据

    *illuminance = handle->illuminance;
    handle->data_flag = 0; // 清除标志位，状态保持不变
    return 0;
}

// GY30状态机驱动函数
void gy30_run(GY30_Handle *handle) {
    if (!handle) return;

    // 检查重试次数是否达到上限
    if (handle->retry_count >= GY30_MAX_RETRY_COUNT) {
        // 重试次数达到上限，重置为初始状态，让用户可以重新初始化
        handle->status = GY30_EMPTY;
        handle->retry_count = 0;
        handle->data_flag = 0;
        return;
    }

    switch (handle->status) {
        case GY30_EMPTY:
            // 初始状态，等待初始化
            // 单次模式下，用户调用gy30_set_mode后会进入READY状态
            break;
        case GY30_READY:
            // 准备开始测量，发送测量命令
            if (_gy30_send_command(handle, handle->mode) == GY30_OK) {
                handle->status = GY30_START;
                handle->start_time = GET_TIME_MS(); // 记录开始时间
                handle->retry_count = 0; // 重置重试计数
            }
            else {
                // 发送失败，增加重试计数并重置状态
                handle->retry_count++;
                handle->status = GY30_EMPTY;
            }
            break;

        case GY30_START:
            // 已发送测量命令，根据模式等待测量时间
            {
                uint32_t measure_time = (handle->mode == GY30_CONT_LR_MODE ||
                                         handle->mode == GY30_ONE_LR_MODE) ?
                                         GY30_MEASURE_TIME_LR : GY30_MEASURE_TIME_HR;

                if (GET_TIME_MS() - handle->start_time >= measure_time) {
                    handle->status = GY30_READ;
                }
            }
            break;

        case GY30_READ:
            // 读取测量数据
            if (_gy30_read_data(handle) == GY30_OK) {
                _gy30_process_data(handle);
                handle->status = GY30_COMPLETE;
                handle->retry_count = 0; // 重置重试计数
            }
            else {
                handle->retry_count++;
            }
            break;

        case GY30_COMPLETE:
            // 数据校验
            if (_gy30_check_data(handle) == GY30_OK) {
                // 数据有效，保持在COMPLETE状态等待用户读取
                // 用户调用gy30_get后data_flag会被清零
                if (handle->data_flag == 0) {
                    // 数据已被读取，准备下一次测量
                    if (handle->mode == GY30_CONT_HR_MODE ||
                        handle->mode == GY30_CONT_HR_MODE2 ||
                        handle->mode == GY30_CONT_LR_MODE) {
                        // 连续模式：重新开始测量
                        handle->status = GY30_START;
                        handle->start_time = GET_TIME_MS();
                    } else {
                        // 单次模式：回到初始状态
                        handle->status = GY30_EMPTY;
                    }
                }
            } else {
                // 数据无效，增加重试计数并重新测量
                handle->retry_count++;
                handle->status = GY30_START;
                handle->start_time = GET_TIME_MS(); // 重新开始计时
                handle->data_flag = 0;
            }
            break;

        default:
            // 无效状态，重置为初始状态
            handle->status = GY30_EMPTY;
            break;
    }
}

int gy30_ready(GY30_Handle *handle) {
    if (!handle) return -1;
    return (handle->data_flag == 1) ? 0 : -2; // 0: ready, -2: no data
}