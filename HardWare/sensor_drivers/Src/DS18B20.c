#include "func_config.h"
#include "main.h"
#include "DS18B20.h"

// 设置GPIO为输出模式
static void _ds18b20_output_mode(DS18B20_Handle *handle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = handle->gpio.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 推挽输出
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(handle->gpio.port, &GPIO_InitStruct);
}

// 设置GPIO为输入模式（带上拉）
static void _ds18b20_input_mode(DS18B20_Handle *handle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = handle->gpio.pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // 输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // 上拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(handle->gpio.port, &GPIO_InitStruct);
}


// 发送复位脉冲并检测存在脉冲
static eDS18B20_error_code _ds18b20_reset(DS18B20_Handle *handle) {
    switch (handle->reset_status) {
        case SET_LOW_LEVEL:
            _ds18b20_output_mode(handle);
            GPIO_SET_LOW(handle->gpio);  // 拉低总线开始复位
            handle->reset_start_time_us = GET_TIME_US();  // 记录开始时间（微秒）
            handle->reset_status = SET_HIGH_LEVEL;
            break;

        case SET_HIGH_LEVEL:
            if ((uint32_t)(GET_TIME_US() - handle->reset_start_time_us) >= 480U) {  // 保持至少480us低电平
                _ds18b20_input_mode(handle);  // 释放总线
                handle->reset_start_time_us = GET_TIME_US();  // 重新记录时间等待存在脉冲
                handle->reset_status = WAIT_RESPONSE;
            }
            break;

        case WAIT_RESPONSE:
            /* 在释放总线后，presence 脉冲应在15..240us之间出现并保持一段时间
             * 首次在 >=70us 时采样，如果检测到低电平则判定存在；
             * 若超过 300us 仍无响应，则认为无设备，统计失败并重试。 */
            if ((uint32_t)(GET_TIME_US() - handle->reset_start_time_us) >= 70U) {
                if (GPIO_READ_LEVEL(handle->gpio) == GPIO_PIN_RESET) {
                    handle->presence = 1;  // 检测到存在脉冲
                    handle->reset_status = RESET_COMPLETE;
                } else if ((uint32_t)(GET_TIME_US() - handle->reset_start_time_us) >= 300U) {
                    handle->presence = 0;
                    handle->reset_fail_count++;
                    handle->reset_status = SET_LOW_LEVEL; // 重置状态机准备下一次复位
                }
            }
            break;

        case RESET_COMPLETE:
            /* 等待 presence 脉冲结束（总线回到高电平）再返回 OK */
            if (GPIO_READ_LEVEL(handle->gpio) == GPIO_PIN_SET) {
                handle->reset_fail_count = 0; // 成功后清除失败计数
                return DS18B20_OK;
            }
            break;
    }

    return DS18B20_BUSY;  // 复位过程仍在进行中
}

// 写一位到DS18B20
static void _ds18b20_write_bit(DS18B20_Handle *handle, uint8_t bit) {
    _ds18b20_output_mode(handle);
    GPIO_SET_LOW(handle->gpio);  // 拉低总线开始写时隙

    if (bit) {
        // 写1：拉低1us后释放总线
        DELAY_US(1);
        _ds18b20_input_mode(handle);  // 释放总线
        DELAY_US(60);  // 保持高电平
    } else {
        // 写0：保持低电平60us
        DELAY_US(60);
        _ds18b20_input_mode(handle);  // 释放总线
        DELAY_US(1);  // 恢复时间
    }
}

// 从DS18B20读取一位
static uint8_t _ds18b20_read_bit(DS18B20_Handle *handle) {
    uint8_t bit = 0;

    _ds18b20_output_mode(handle);
    GPIO_SET_LOW(handle->gpio);  // 拉低总线开始读时隙
    DELAY_US(1);  // 保持低电平至少1us

    _ds18b20_input_mode(handle);  // 释放总线

    DELAY_US(10);  // 等待10-15us后采样

    // 采样总线电平
    if (GPIO_READ_LEVEL(handle->gpio) == GPIO_PIN_SET) {
        bit = 1;  // 高电平表示读1
    } else {
        bit = 0;  // 低电平表示读0
    }

    DELAY_US(50);  // 完成读时隙

    return bit;
}

// 写一个字节到DS18B20（LSB first）
static void _ds18b20_write_byte(DS18B20_Handle *handle, uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        _ds18b20_write_bit(handle, byte & 0x01);
        byte >>= 1;
    }
}

// 从DS18B20读取一个字节（LSB first）
static uint8_t _ds18b20_read_byte(DS18B20_Handle *handle) {
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++) {
        byte >>= 1;
        if (_ds18b20_read_bit(handle)) {
            byte |= 0x80;  // LSB first，所以先读的放低位
        }
    }
    return byte;
}

// 计算CRC8校验
static uint8_t _ds18b20_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;  // CRC多项式：x^8 + x^5 + x^4 + 1
            }
            byte >>= 1;
        }
    }
    return crc;
}

// 读取暂存器（9字节）
static eDS18B20_error_code _ds18b20_read_scratchpad(DS18B20_Handle *handle) {
    for (uint8_t i = 0; i < 9; i++) {
        handle->scratchpad[i] = _ds18b20_read_byte(handle);
    }

    // 校验CRC
    uint8_t crc = _ds18b20_crc8(handle->scratchpad, 8);
    if (crc != handle->scratchpad[8]) {
        return DS18B20_ERROR_CRC;
    }

    return DS18B20_OK;
}

// 处理温度数据
static void _ds18b20_process_data(DS18B20_Handle *handle) {
    // 从暂存器获取原始温度值（低字节在前）
    uint16_t rawu = (uint16_t)handle->scratchpad[0] | ((uint16_t)handle->scratchpad[1] << 8);

    // 根据分辨率清除无效的低位，避免未定义位引入随机小数
    uint8_t bits_to_clear = 0;
    switch (handle->resolution) {
        case DS18B20_RESOLUTION_9BIT:  bits_to_clear = 3; break;
        case DS18B20_RESOLUTION_10BIT: bits_to_clear = 2; break;
        case DS18B20_RESOLUTION_11BIT: bits_to_clear = 1; break;
        case DS18B20_RESOLUTION_12BIT:
        default:                        bits_to_clear = 0; break;
    }
    if (bits_to_clear) {
        rawu &= (uint16_t)(~((1u << bits_to_clear) - 1u)); // 清除无效位
    }

    // 将无符号原始值解释为有符号二进制补码
    int16_t raw_temp = (int16_t)rawu;

    // 温度换算：寄存器单位为 1/16 °C（12-bit LSB = 0.0625），对所有分辨率通用
    float temperature = (float)raw_temp * DS18B20_LSB_12BIT;
    handle->temperature = temperature;
    handle->data_flag = 1;  // 数据准备好
}

// 检查温度数据是否在合理范围内
static eDS18B20_error_code _ds18b20_check_data(DS18B20_Handle *handle) {
    // DS18B20测量范围：-55°C ~ +125°C
    if (handle->temperature < -55.0f || handle->temperature > 125.0f) {
        handle->data_flag = 0;
        return DS18B20_ERROR_DATA;
    }

    // 检查温度值是否为85°C（上电默认值，可能表示错误）
    if (handle->temperature == 85.0f) {
        handle->data_flag = 0;
        return DS18B20_ERROR_DATA;
    }

    return DS18B20_OK;
}

// 获取转换时间（根据分辨率）
static uint32_t _ds18b20_get_conversion_time(DS18B20_Handle *handle) {
    switch (handle->resolution) {
        case DS18B20_RESOLUTION_9BIT:
            return DS18B20_CONV_TIME_9BIT;
        case DS18B20_RESOLUTION_10BIT:
            return DS18B20_CONV_TIME_10BIT;
        case DS18B20_RESOLUTION_11BIT:
            return DS18B20_CONV_TIME_11BIT;
        case DS18B20_RESOLUTION_12BIT:
        default:
            return DS18B20_CONV_TIME_12BIT;
    }
}

// DS18B20初始化函数
int ds18b20_init(DS18B20_Handle *handle) {
    if (!handle) return -1;

    // 初始状态为INIT
    handle->status = DS18B20_INIT;
    handle->use_rom = 0;  // 默认跳过ROM
    handle->resolution = DS18B20_DEFAULT_RESOLUTION;
    handle->data_flag = 0;
    handle->retry_count = 0;
    handle->conversion_done = 0;
    handle->start_time = 0;

    // 初始化ROM码为0（使用跳过ROM）
    for (uint8_t i = 0; i < 8; i++) {
        handle->rom_code[i] = 0;
    }

    return 0;  // 初始化成功
}

// 获取已准备好的数据
int ds18b20_get(DS18B20_Handle *handle, float *temperature) {
    if (!handle || !temperature) return -1;
    if (handle->data_flag != 1) return -2;  // 无数据

    *temperature = handle->temperature;
    handle->data_flag = 0;  // 清除标志位
    return 0;
}

// 设置分辨率（只更新handle，下次测量生效）
int ds18b20_set_resolution(DS18B20_Handle *handle, uint8_t resolution) {
    if (!handle) return -1;

    // 检查分辨率参数是否有效
    if (resolution != DS18B20_RESOLUTION_9BIT &&
        resolution != DS18B20_RESOLUTION_10BIT &&
        resolution != DS18B20_RESOLUTION_11BIT &&
        resolution != DS18B20_RESOLUTION_12BIT) {
        return -2;  // 无效的分辨率参数
    }

    // 更新分辨率设置
    handle->resolution = resolution;

    // 如果当前正在等待转换，可能需要重置转换时间
    // 这里只更新设置，下次测量会自动使用新分辨率

    return 0;  // 成功
}

// DS18B20状态机驱动函数
void ds18b20_run(DS18B20_Handle *handle) {
    if (!handle) return;

    // 重试机制：达到最大重试次数时的处理
    // if (handle->retry_count >= 3) {
    
    // }

    switch (handle->status) {
        case DS18B20_EMPTY:
            // 初始状态，等待初始化
            // 用户可以调用ds18b20_init来初始化
            break;

        case DS18B20_INIT:
            // 发送复位脉冲并检测存在脉冲

            if (_ds18b20_reset(handle) == DS18B20_OK) {
                if (handle->conversion_done) {
                    // 转换已完成，准备读取数据
                    handle->status = DS18B20_READ_CMD;
                    handle->conversion_done = 0;  // 清除标志
                } else {
                    // 开始新的转换流程
                    handle->status = DS18B20_ROM_CMD;
                }
            }
            break;

        case DS18B20_ROM_CMD:
            // 发送ROM命令
            if (handle->use_rom) {
                // 匹配ROM：发送匹配ROM命令 + ROM码
                _ds18b20_write_byte(handle, DS18B20_CMD_MATCH_ROM);
                for (uint8_t i = 0; i < 8; i++) {
                    _ds18b20_write_byte(handle, handle->rom_code[i]);
                }
            } else {
                // 跳过ROM
                _ds18b20_write_byte(handle, DS18B20_CMD_SKIP_ROM);
            }
            handle->status = DS18B20_CONVERT;
            break;

        case DS18B20_CONVERT:
            // 发送温度转换命令
            _ds18b20_write_byte(handle, DS18B20_CMD_CONVERT_T);
            handle->status = DS18B20_WAIT_CONV;
            handle->start_time = GET_TIME_MS();  // 记录开始时间
            break;

        case DS18B20_WAIT_CONV:
            // 等待温度转换完成
            {
                uint32_t conv_time = _ds18b20_get_conversion_time(handle);
                if (GET_TIME_MS() - handle->start_time >= conv_time) {
                    // 转换完成，设置标志并重新初始化总线
                    handle->conversion_done = 1;
                    handle->status = DS18B20_INIT;
                }
            }
            break;

        case DS18B20_READ_CMD:
            // 发送ROM命令（与转换时间相同）
            if (handle->use_rom) {
                // 匹配ROM：发送匹配ROM命令 + ROM码
                _ds18b20_write_byte(handle, DS18B20_CMD_MATCH_ROM);
                for (uint8_t i = 0; i < 8; i++) {
                    _ds18b20_write_byte(handle, handle->rom_code[i]);
                }
            } else {
                // 跳过ROM
                _ds18b20_write_byte(handle, DS18B20_CMD_SKIP_ROM);
            }
            // 发送读取暂存器命令
            _ds18b20_write_byte(handle, DS18B20_CMD_READ_SCRATCH);
            handle->status = DS18B20_READ_DATA;
            break;

        case DS18B20_READ_DATA:
            // 读取温度数据
            if (_ds18b20_read_scratchpad(handle) == DS18B20_OK) {
                _ds18b20_process_data(handle);
                handle->status = DS18B20_COMPLETE;
            } else {
                // CRC错误，重试读取（转换已完成，无需重新转换）
                handle->retry_count++;
                handle->conversion_done = 1;  // 标记转换已完成
                handle->status = DS18B20_INIT;  // 从复位重新开始读取
            }
            break;

        case DS18B20_COMPLETE:
            // 数据校验
            if (_ds18b20_check_data(handle) == DS18B20_OK) {
                // 数据有效，保持在COMPLETE状态等待用户读取
                // 用户调用ds18b20_get后data_flag会被清零
                if (handle->data_flag == 0) {
                    // 数据已被读取，回到初始状态等待下次初始化
                    handle->status = DS18B20_EMPTY;
                }
            } else {
                // 数据无效，重试读取（转换已完成，无需重新转换）
                handle->retry_count++;
                handle->conversion_done = 1;  // 标记转换已完成
                handle->status = DS18B20_INIT;
                handle->data_flag = 0;
            }
            break;

        default:
            // 无效状态，重置为初始状态
            handle->status = DS18B20_EMPTY;
            break;
    }
}

int ds18b20_ready(DS18B20_Handle *handle) {
    if (!handle) return -1;
    return (handle->data_flag == 1) ? 0 : -2; // 0: ready, -2: no data
}