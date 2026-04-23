# 传感器驱动命名规范

## 文件命名
- 头文件: `设备名.h` (如: `DS3231.h`)
- 源文件: `设备名.c` (如: `DS3231.c`)

## 数据结构

### 状态枚举
```c
typedef enum {
    设备名_xxx,
    // 根据传感器不同灵活拆分
} e设备名_status;
```

### 错误码枚举
```c
typedef enum {
    设备名_OK = 0,         // 操作成功
    设备名_ERROR_XXX,      // 具体错误类型
    设备名_ERROR_YYY,
} e设备名_error_code;
```

### 句柄结构
```c
typedef struct {
    通信接口_handle 接口;   // 如: I2C_handle, GPIO_handle
    e设备名_status status;  // 当前状态
    uint8_t data_flag;     // 数据准备好标志 (0:无数据, 1:数据就绪)
    uint32_t start_time;   // 时间戳 (ms)
    // ... 设备特定字段
    类型 数据字段;          // 存储测量数据
} 设备名_Handle;
```

## 函数命名

### 公共API
- `设备名_init()` - 初始化驱动
- `设备名_run()` - 状态机驱动函数（非阻塞，需定期调用）
- `设备名_get()` - 获取已准备好的数据

### 内部函数
- `_设备名_xxx()` - 内部实现函数（static）

## 宏定义
- `设备名_XXX_YYY` - 设备相关常量（命令字、地址等）

## 示例模板

### 头文件模板
```c
/* 设备名.h - 设备描述 */
#ifndef 设备名_H
#define 设备名_H

#include <stdint.h>
#include "func_config.h"

// 设备常量定义
#define 设备名_ADDR 0x00

// 状态枚举
typedef enum {
    设备名_EMPTY = 0,
    // ... 其他状态
    设备名_COMPLETE,
} e设备名_status;

// 错误码枚举  
typedef enum {
    设备名_OK = 0,
    设备名_ERROR_XXX,
    // ... 其他错误
} e设备名_error_code;

// 句柄结构
typedef struct {
    I2C_handle i2c;           // 或 GPIO_handle gpio
    e设备名_status status;
    uint8_t data_flag;
    uint32_t start_time;
    // ... 设备特定字段
} 设备名_Handle;

// 公共API
int 设备名_init(设备名_Handle *handle);
void 设备名_run(设备名_Handle *handle);
int 设备名_get(设备名_Handle *handle, 类型 *data);

#endif // 设备名_H
```

### 源文件模板
```c
#include "main.h"
#include "设备名.h"

// 内部函数实现
static e设备名_error_code _设备名_内部函数(设备名_Handle *handle) {
    // 实现
    return 设备名_OK;
}

// 数据处理
static void _设备名_process_data(设备名_Handle *handle) {
    // 处理原始数据
    handle->data_flag = 1;
}

// 初始化
int 设备名_init(设备名_Handle *handle) {
    if (!handle) return -1;
    handle->status = ...;
    handle->data_flag = 0;
    return 0;
}

// 获取数据
int 设备名_get(设备名_Handle *handle, 类型 *data) {
    if (!handle || !data) return -1;
    if (handle->data_flag != 1) return -2;
    *data = handle->数据字段;
    handle->data_flag = 0;
    return 0;
}

// 状态机
void 设备名_run(设备名_Handle *handle) {
    if (!handle) return;
    
    // 此处可加错误处理，如重启等

    switch (handle->status) {
        // 根据传感器不同灵活调整
        case 设备名_COMPLETE:
            // 数据就绪
            break;
        default:
            handle->status = 设备名_EMPTY;
            break;
    }
}
```