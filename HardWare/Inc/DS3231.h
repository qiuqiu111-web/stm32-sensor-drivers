/* DS3231.h - DS3231 real-time clock driver public types and API */
#ifndef DS3231_H
#define DS3231_H

#include <stdint.h>
#include "func_config.h"

// DS3231 I2C地址（7位地址，HAL库会自动左移1位）
#define DS3231_I2C_ADDR  0x68  // 7位地址：01101000

// DS3231寄存器地址
#define DS3231_REG_SECONDS     0x00  // 秒（00-59）
#define DS3231_REG_MINUTES     0x01  // 分（00-59）
#define DS3231_REG_HOURS       0x02  // 时（00-23或带AM/PM标志）
#define DS3231_REG_DAY         0x03  // 星期（1-7，1=星期日）
#define DS3231_REG_DATE        0x04  // 日（01-31）
#define DS3231_REG_MONTH       0x05  // 月（01-12）
#define DS3231_REG_YEAR        0x06  // 年（00-99）
#define DS3231_REG_CONTROL     0x0E  // 控制寄存器
#define DS3231_REG_STATUS      0x0F  // 状态寄存器
#define DS3231_REG_TEMP_MSB    0x11  // 温度高字节
#define DS3231_REG_TEMP_LSB    0x12  // 温度低字节

// 时间格式标志（小时寄存器）
#define DS3231_HOUR_12H_FORMAT 0x40  // 12小时制标志
#define DS3231_HOUR_PM_FLAG    0x20  // PM标志（12小时制下）
#define DS3231_HOUR_24H_MASK   0x3F  // 24小时制掩码

// 控制寄存器位
#define DS3231_CTRL_EOSC       0x80  // 振荡器使能（0=使能，1=停止）
#define DS3231_CTRL_BBSQW      0x40  // Battery-Backed Square-Wave Enable
#define DS3231_CTRL_CONV       0x20  // 转换温度使能
#define DS3231_CTRL_RS2        0x10  // 速率选择位2
#define DS3231_CTRL_RS1        0x08  // 速率选择位1
#define DS3231_CTRL_INTCN      0x04  // 中断控制
#define DS3231_CTRL_A2IE       0x02  // 闹钟2中断使能
#define DS3231_CTRL_A1IE       0x01  // 闹钟1中断使能

// 状态寄存器位
#define DS3231_STATUS_OSF      0x80  // 振荡器停止标志
#define DS3231_STATUS_EN32KHZ  0x08  // 32kHz输出使能
#define DS3231_STATUS_BSY      0x04  // 忙标志（温度转换中）
#define DS3231_STATUS_A2F      0x02  // 闹钟2标志
#define DS3231_STATUS_A1F      0x01  // 闹钟1标志

// 默认控制寄存器值（使能振荡器，禁用中断）
#define DS3231_DEFAULT_CONTROL 0x00

typedef enum {
    DS3231_INIT,           // 初始化状态（配置控制寄存器）
    DS3231_READ_TIME,      // 读取时间数据
    DS3231_WAIT_READ,      // 等待读取完成
    DS3231_COMPLETE,       // 数据就绪
    DS3231_SET_TIME,       // 设置时间
} eDS3231_status;

typedef enum {
    DS3231_OK = 0,         // 操作成功
    DS3231_ERROR_I2C,      // I2C通信错误
    DS3231_ERROR_TIMEOUT,  // 超时错误
    DS3231_ERROR_DATA,     // 数据错误（无效时间）
    DS3231_ERROR_OSF,      // 振荡器停止错误
} eDS3231_error_code;

// 时间数据结构（BCD格式转换后）
typedef struct {
    uint8_t seconds;       // 秒 (0-59)
    uint8_t minutes;       // 分 (0-59)
    uint8_t hours;         // 时 (0-23，24小时制)
    uint8_t day_of_week;   // 星期 (1-7，1=星期日)
    uint8_t date;          // 日 (1-31)
    uint8_t month;         // 月 (1-12)
    uint8_t year;          // 年 (0-99)
    uint8_t century;       // 世纪标志 (0=20xx，1=21xx)
    float temperature;     // 温度（℃）
} DS3231_Time;

// DS3231句柄结构
typedef struct {
    I2C_handle i2c;               // I2C配置
    eDS3231_status status;        // 当前状态
    uint8_t control_reg;          // 控制寄存器值
    uint8_t status_reg;           // 状态寄存器值
    uint8_t raw_time[7];          // 原始时间数据（7字节）
    DS3231_Time time;             // 解析后的时间
    uint8_t retry_count;          // 重试次数
    uint8_t data_flag;            // 数据准备好标志 (0:无数据, 1:时间就绪)
    uint8_t temperature_flag;     // 温度数据标志 (0:无温度, 1:温度就绪)
    uint32_t start_time;          // 时间戳（ms）
} DS3231_Handle;

extern DS3231_Time ds3231_time; // 全局时间变量

// 初始化，返回 0 成功
int ds3231_init(DS3231_Handle *handle);

// 状态机驱动，非阻塞，需定期调用
void ds3231_run(DS3231_Handle *handle);

// 获取已准备好的时间数据（若无数据返回非 0）
int ds3231_get_time(DS3231_Handle *handle, DS3231_Time *time);

// 数据就绪检查，返回 0 表示数据准备好
int ds3231_ready(DS3231_Handle *handle);

// 获取温度数据
// int ds3231_get_temperature(DS3231_Handle *handle, float *temperature);

// 设置时间
int ds3231_set_time(DS3231_Handle *handle, const DS3231_Time *time);

#endif // DS3231_H