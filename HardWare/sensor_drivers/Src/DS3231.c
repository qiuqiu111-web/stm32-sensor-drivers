#include "main.h"
#include "stm32f4xx_hal_i2c.h"
#include "DS3231.h"

// 确保I2C内存地址大小常量已定义
#ifndef I2C_MEMADD_SIZE_8BIT
#define I2C_MEMADD_SIZE_8BIT 1
#endif
#ifndef I2C_MEMADD_SIZE_16BIT
#define I2C_MEMADD_SIZE_16BIT 2
#endif

DS3231_Time ds3231_time = {
    .seconds = 0,
    .minutes = 0,
    .hours = 0,
    .day_of_week = 4,
    .date = 23,
    .month = 4,
    .year = 26,
    .century = 1,
}; // 全局时间变量

// BCD码转十进制
static uint8_t _bcd_to_dec(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// 十进制转BCD码
static uint8_t _dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

// 读取DS3231寄存器
static eDS3231_error_code _ds3231_read_registers(DS3231_Handle *handle, uint8_t reg_addr, uint8_t *data, uint8_t size) {
    HAL_StatusTypeDef hal_status = I2C_MEM_READ(handle->i2c, reg_addr, I2C_MEMADD_SIZE_8BIT, data, size);
    if (hal_status != HAL_OK) {
        return DS3231_ERROR_I2C;
    }
    return DS3231_OK;
}

// 写入DS3231寄存器
static eDS3231_error_code _ds3231_write_registers(DS3231_Handle *handle, uint8_t reg_addr, const uint8_t *data, uint8_t size) {
    HAL_StatusTypeDef hal_status = I2C_MEM_WRITE(handle->i2c, reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t*)data, size);
    if (hal_status != HAL_OK) {
        return DS3231_ERROR_I2C;
    }
    return DS3231_OK;
}

// 读取状态寄存器
static eDS3231_error_code _ds3231_read_status(DS3231_Handle *handle) {
    uint8_t status_byte;
    eDS3231_error_code err = _ds3231_read_registers(handle, DS3231_REG_STATUS, &status_byte, 1);
    if (err != DS3231_OK) {
        return err;
    }

    handle->status_reg = status_byte;

    // 检查振荡器停止标志
    if (status_byte & DS3231_STATUS_OSF) {
        return DS3231_ERROR_OSF;
    }

    return DS3231_OK;
}

// 读取控制寄存器
static eDS3231_error_code _ds3231_read_control(DS3231_Handle *handle) {
    uint8_t control_byte;
    eDS3231_error_code err = _ds3231_read_registers(handle, DS3231_REG_CONTROL, &control_byte, 1);
    if (err != DS3231_OK) {
        return err;
    }

    handle->control_reg = control_byte;
    return DS3231_OK;
}

// 写入控制寄存器
static eDS3231_error_code _ds3231_write_control(DS3231_Handle *handle, uint8_t control_value) {
    eDS3231_error_code err = _ds3231_write_registers(handle, DS3231_REG_CONTROL, &control_value, 1);
    if (err != DS3231_OK) {
        return err;
    }

    handle->control_reg = control_value;
    return DS3231_OK;
}

// 读取时间数据（7字节）
static eDS3231_error_code _ds3231_read_time_data(DS3231_Handle *handle) {
    return _ds3231_read_registers(handle, DS3231_REG_SECONDS, handle->raw_time, 7);
}

// 读取温度数据（2字节）
static eDS3231_error_code _ds3231_read_temperature(DS3231_Handle *handle) {
    uint8_t temp_data[2];
    eDS3231_error_code err = _ds3231_read_registers(handle, DS3231_REG_TEMP_MSB, temp_data, 2);
    if (err != DS3231_OK) {
        return err;
    }

    // 温度数据格式：高字节为整数部分（带符号），低字节高2位为小数部分（0.25℃分辨率）
    int8_t msb = (int8_t)temp_data[0];
    uint8_t lsb = temp_data[1];
    float temperature = (float)msb + ((lsb >> 6) * 0.25f);

    handle->time.temperature = temperature;
    handle->temperature_flag = 1;

    return DS3231_OK;
}

// 解析时间数据（BCD转十进制，处理格式）
static void _ds3231_parse_time(DS3231_Handle *handle) {
    uint8_t *raw = handle->raw_time;

    // 解析秒（BCD）
    handle->time.seconds = _bcd_to_dec(raw[0] & 0x7F);  // 忽略最高位（时钟停止位）

    // 解析分（BCD）
    handle->time.minutes = _bcd_to_dec(raw[1] & 0x7F);  // 忽略最高位

    // 解析小时（处理12/24小时制）
    if (raw[2] & DS3231_HOUR_12H_FORMAT) {
        // 12小时制
        uint8_t hour12 = _bcd_to_dec(raw[2] & 0x1F);  // 低5位
        if (raw[2] & DS3231_HOUR_PM_FLAG) {
            // PM：12小时制下，12PM=12:00, 1PM=13:00, ... 11PM=23:00
            handle->time.hours = (hour12 == 12) ? 12 : (hour12 + 12);
        } else {
            // AM：12AM=00:00, 1AM=01:00, ... 11AM=11:00
            handle->time.hours = (hour12 == 12) ? 0 : hour12;
        }
    } else {
        // 24小时制
        handle->time.hours = _bcd_to_dec(raw[2] & DS3231_HOUR_24H_MASK);
    }

    // 解析星期（1-7，1=星期日）
    handle->time.day_of_week = _bcd_to_dec(raw[3] & 0x07);

    // 解析日（BCD）
    handle->time.date = _bcd_to_dec(raw[4] & 0x3F);  // 忽略世纪标志位

    // 解析月（BCD，包含世纪标志）
    handle->time.month = _bcd_to_dec(raw[5] & 0x1F);  // 低5位为月份
    handle->time.century = (raw[5] & 0x80) ? 1 : 0;    // 最高位为世纪标志

    // 解析年（BCD）
    handle->time.year = _bcd_to_dec(raw[6]);

    // 标记数据就绪
    handle->data_flag = 1;
}

// 检查时间数据是否合理（通用函数）
static eDS3231_error_code _ds3231_check_time_struct(const DS3231_Time *time) {
    if (!time) return DS3231_ERROR_DATA;

    // 基本范围检查
    if (time->seconds > 59 || time->minutes > 59 || time->hours > 23 ||
        time->day_of_week < 1 || time->day_of_week > 7 ||
        time->date < 1 || time->date > 31 ||
        time->month < 1 || time->month > 12 ||
        time->year > 99) {
        return DS3231_ERROR_DATA;
    }

    // 简单月份天数检查（考虑闰年）
    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (time->date > days_in_month[time->month - 1]) {
        return DS3231_ERROR_DATA;
    }

    return DS3231_OK;
}

// 检查句柄中的时间数据是否合理
static eDS3231_error_code _ds3231_check_time(DS3231_Handle *handle) {
    return _ds3231_check_time_struct(&handle->time);
}

// DS3231初始化函数
int ds3231_init(DS3231_Handle *handle) {
    if (!handle) return -1;

    // 初始状态为INIT
    handle->status = DS3231_INIT;
    handle->data_flag = 0;
    handle->temperature_flag = 0;
    handle->retry_count = 0;
    handle->start_time = 0;
    handle->control_reg = DS3231_DEFAULT_CONTROL;

    // 预设一个默认时间
    handle->time = ds3231_time;

    return 0;  // 初始化成功
}

// 获取已准备好的时间数据
int ds3231_get_time(DS3231_Handle *handle, DS3231_Time *time) {
    if (!handle || !time) return -1;
    if (handle->data_flag != 1) return -2;  // 无数据

    *time = handle->time;
    handle->data_flag = 0;  // 清除标志位
    return 0;
}

// 获取温度数据
int ds3231_get_temperature(DS3231_Handle *handle, float *temperature) {
    if (!handle || !temperature) return -1;
    if (handle->temperature_flag != 1) return -2;  // 无温度数据

    *temperature = handle->time.temperature;
    handle->temperature_flag = 0;  // 清除标志位
    return 0;
}

// DS3231状态机驱动函数（简化版）
void ds3231_run(DS3231_Handle *handle) {
    if (!handle) return;

    // 重试机制：达到最大重试次数时，回到INIT重新开始
    if (handle->retry_count >= 3) {
        handle->retry_count = 0;
        handle->status = DS3231_INIT;
    }

    switch (handle->status) {

        case DS3231_INIT:
            {
                // 读取状态寄存器，检查振荡器
                eDS3231_error_code err = _ds3231_read_status(handle);
                if (err == DS3231_OK) {
                    // 读取控制寄存器以同步真实值，再决定下一步
                    if (_ds3231_read_control(handle) == DS3231_OK) {
                        if (handle->control_reg != DS3231_DEFAULT_CONTROL) {
                            handle->status = DS3231_SET_TIME;
                        } else {
                            handle->status = DS3231_READ_TIME;
                        }
                        handle->retry_count = 0;
                    } else {
                        handle->retry_count++;
                    }
                } else if (err == DS3231_ERROR_OSF) {
                    // 振荡器停止标志（OSF）被置位：尝试清除 OSF 并继续读取时间
                    uint8_t new_status = handle->status_reg & ~DS3231_STATUS_OSF;
                    if (_ds3231_write_registers(handle, DS3231_REG_STATUS, &new_status, 1) == DS3231_OK) {
                        handle->status_reg = new_status;
                        handle->retry_count = 0;
                        handle->status = DS3231_READ_TIME; // 尝试读取时间（时间可能不可靠）
                    } else {
                        handle->retry_count++;
                    }
                } else {
                    handle->retry_count++;
                }
                break;
            }

        case DS3231_READ_TIME:
            // 直接读取并解析时间，立即校验（去掉不必要的 WAIT_READ/延迟）
            if (_ds3231_read_time_data(handle) == DS3231_OK) {
                _ds3231_parse_time(handle);
                if (_ds3231_check_time(handle) == DS3231_OK) {
                    // 读取温度（非必需，忽略错误）
                    (void)_ds3231_read_temperature(handle);
                    handle->data_flag = 1;  // 标记数据就绪
                    handle->status = DS3231_COMPLETE;
                } else {
                    // 时间数据不合理，重试
                    handle->retry_count++;
                    handle->status = DS3231_INIT;
                }
            } else {
                handle->retry_count++;
                handle->status = DS3231_INIT;
            }
            break;

        case DS3231_COMPLETE:
            // 等待用户读取数据（ds3231_get_time 会清除 data_flag）
            if (handle->data_flag == 0) {
                handle->status = DS3231_READ_TIME;
            }
            break;

        case DS3231_SET_TIME:
            // 写入控制寄存器后返回 INIT
            if (_ds3231_write_control(handle, handle->control_reg) == DS3231_OK) {
                handle->status = DS3231_INIT;
                handle->retry_count = 0;
            } else {
                handle->retry_count++;
            }
            break;

        default:
            handle->status = DS3231_INIT;
            break;
    }
}


// 将时间结构转换为原始寄存器数据
static void _ds3231_time_to_raw(const DS3231_Time *time, uint8_t *raw_data) {
    // 秒（BCD格式，最高位为时钟停止标志，通常设为0）
    raw_data[0] = _dec_to_bcd(time->seconds) & 0x7F;  // 确保时钟运行

    // 分（BCD格式）
    raw_data[1] = _dec_to_bcd(time->minutes) & 0x7F;

    // 时（使用24小时制，BCD格式）
    raw_data[2] = _dec_to_bcd(time->hours) & DS3231_HOUR_24H_MASK;

    // 星期（1-7，1=星期日）
    raw_data[3] = _dec_to_bcd(time->day_of_week) & 0x07;

    // 日（BCD格式）
    raw_data[4] = _dec_to_bcd(time->date) & 0x3F;

    // 月（BCD格式，包含世纪标志）
    raw_data[5] = _dec_to_bcd(time->month) & 0x1F;
    if (time->century) {
        raw_data[5] |= 0x80;  // 设置世纪标志
    }

    // 年（BCD格式）
    raw_data[6] = _dec_to_bcd(time->year);
}

// 设置时间（同步操作，直接写入寄存器）
int ds3231_set_time(DS3231_Handle *handle, const DS3231_Time *time) {
    if (!handle || !time) return -1;

    // 验证时间数据
    eDS3231_error_code err = _ds3231_check_time_struct(time);
    if (err != DS3231_OK) {
        return -2;  // 无效时间数据
    }

    // 将时间转换为原始寄存器数据
    uint8_t raw_time[7];
    _ds3231_time_to_raw(time, raw_time);

    // 写入时间寄存器（从秒寄存器开始，连续7个寄存器）
    err = _ds3231_write_registers(handle, DS3231_REG_SECONDS, raw_time, 7);
    if (err != DS3231_OK) {
        return -3;  // I2C写入失败
    }

    // 清除振荡器停止标志（如果设置了）
    if (handle->status_reg & DS3231_STATUS_OSF) {
        uint8_t new_status = handle->status_reg & ~DS3231_STATUS_OSF;
        err = _ds3231_write_registers(handle, DS3231_REG_STATUS, &new_status, 1);
        if (err == DS3231_OK) {
            handle->status_reg = new_status;
        }
        // 即使清除OSF失败，时间设置也算成功（时间已写入）
    }

    // 更新时间数据到句柄中
    handle->time = *time;
    handle->data_flag = 1;  // 标记数据就绪

    return 0;  // 成功
}


int ds3231_ready(DS3231_Handle *handle) {
    if (!handle) return -1;
    return (handle->data_flag == 1) ? 0 : -2; // 0: ready, -2: no data
}
