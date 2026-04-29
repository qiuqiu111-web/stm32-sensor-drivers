/* DS18B20.h - DS18B20 temperature sensor driver public types and API */
#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include "func_config.h"

// DS18B20命令字
#define DS18B20_CMD_SEARCH_ROM      0xF0
#define DS18B20_CMD_READ_ROM        0x33
#define DS18B20_CMD_MATCH_ROM       0x55
#define DS18B20_CMD_SKIP_ROM        0xCC
#define DS18B20_CMD_ALARM_SEARCH    0xEC
#define DS18B20_CMD_CONVERT_T       0x44
#define DS18B20_CMD_WRITE_SCRATCH   0x4E
#define DS18B20_CMD_READ_SCRATCH    0xBE
#define DS18B20_CMD_COPY_SCRATCH    0x48
#define DS18B20_CMD_RECALL_EE       0xB8
#define DS18B20_CMD_READ_POWER      0xB4

// 分辨率设置（配置寄存器）
#define DS18B20_RESOLUTION_9BIT     0x1F  // 9位分辨率，转换时间93.75ms
#define DS18B20_RESOLUTION_10BIT    0x3F  // 10位分辨率，转换时间187.5ms
#define DS18B20_RESOLUTION_11BIT    0x5F  // 11位分辨率，转换时间375ms
#define DS18B20_RESOLUTION_12BIT    0x7F  // 12位分辨率，转换时间750ms

// 分
/* 各分辨率下每个 LSB 对应的温度（单位：℃） */
#define DS18B20_LSB_9BIT     0.5f      /* 9-bit: 0.5 °C per LSB */
#define DS18B20_LSB_10BIT    0.25f     /* 10-bit: 0.25 °C per LSB */
#define DS18B20_LSB_11BIT    0.125f    /* 11-bit: 0.125 °C per LSB */
#define DS18B20_LSB_12BIT    0.0625f   /* 12-bit: 0.0625 °C per LSB */

// 默认分辨率
#define DS18B20_DEFAULT_RESOLUTION  DS18B20_RESOLUTION_12BIT

// 转换时间（毫秒），根据分辨率不同
#define DS18B20_CONV_TIME_9BIT      94U   // 9位分辨率转换时间
#define DS18B20_CONV_TIME_10BIT     188U  // 10位分辨率转换时间
#define DS18B20_CONV_TIME_11BIT     375U  // 11位分辨率转换时间
#define DS18B20_CONV_TIME_12BIT     750U  // 12位分辨率转换时间

typedef enum {
    DS18B20_EMPTY = 0,      // 初始状态，未初始化
    DS18B20_INIT,           // 初始化状态（发送复位脉冲）
    DS18B20_ROM_CMD,        // 发送ROM命令（跳过ROM或匹配ROM）
    DS18B20_CONVERT,        // 发送温度转换命令
    DS18B20_WAIT_CONV,      // 等待温度转换完成
    DS18B20_READ_CMD,       // 发送读取暂存器命令
    DS18B20_READ_DATA,      // 读取温度数据
    DS18B20_COMPLETE,       // 数据就绪
} eDS18B20_status;

// 复位内部子状态（用于非阻塞复位实现）
typedef enum {
    SET_LOW_LEVEL = 0,
    SET_HIGH_LEVEL,
    WAIT_RESPONSE,
    RESET_COMPLETE,
} eDS_reset_status;

typedef enum {
    DS18B20_OK = 0,         // 操作成功
    DS18B20_ERROR_TIMEOUT,  // 超时错误
    DS18B20_ERROR_PRESENCE, // 无器件响应（无存在脉冲）
    DS18B20_ERROR_CRC,      // CRC校验错误
    DS18B20_ERROR_DATA,     // 数据错误（超出范围）
    DS18B20_BUSY,           // 设备忙（转换未完成）
} eDS18B20_error_code;

typedef struct {
    GPIO_handle gpio;            // GPIO配置
    eDS18B20_status status;      // 当前状态
    eDS_reset_status reset_status;  // 复位状态
    uint8_t rom_code[8];         // ROM码（64位）
    uint8_t use_rom;             // 是否使用ROM码（0:跳过ROM，1:匹配ROM）
    uint8_t resolution;          // 分辨率设置
    uint8_t scratchpad[9];       // 暂存器数据（9字节）
    float temperature;           // 温度值（℃）
    uint8_t retry_count;         // 重试次数
    uint8_t presence;            // presence 检测结果（0/1）
    uint8_t reset_fail_count;    // 连续复位失败计数
    uint8_t data_flag;           // 数据准备好标志
    uint8_t conversion_done;     // 转换完成标志（0:未完成，1:完成）
    uint32_t start_time;         // 时间戳（ms）
    uint32_t reset_start_time_us; // 复位阶段使用的微秒时间戳
} DS18B20_Handle;

// 初始化，返回 0 成功
int ds18b20_init(DS18B20_Handle *handle);

// 状态机驱动，非阻塞，需定期调用
void ds18b20_run(DS18B20_Handle *handle);

// 获取已准备好的数据（若无数据返回非 0）
int ds18b20_get(DS18B20_Handle *handle, float *temperature);

// 数据就绪检查，返回 0 表示数据准备好
int ds18b20_ready(DS18B20_Handle *handle);

// 设置分辨率（只更新handle，下次测量生效）
// 参数：resolution - 分辨率宏（DS18B20_RESOLUTION_9BIT等）
int ds18b20_set_resolution(DS18B20_Handle *handle, uint8_t resolution);

#endif // DS18B20_H