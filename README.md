# STM32F429 传感器驱动集合

基于 STM32F429 的嵌入式传感器驱动学习项目。所有驱动采用**非阻塞状态机**设计，便于集成到 RTOS 或裸机主循环中。

## 传感器列表

| 传感器 | 通信协议 | 驱动文件 | 功能 |
|--------|----------|----------|------|
| DHT22 | 单总线 (GPIO) | `DHT22.c/h` | 温湿度测量 |
| GY30 (BH1750) | I2C | `GY30.c/h` | 光照强度测量 |
| DS18B20 | 1-Wire (GPIO) | `DS18B20.c/h` | 温度测量，支持分辨率配置 |
| DS3231 | I2C | `DS3231.c/h` | 实时时钟（RTC），时间设置与读取 |
| 土壤湿度 (LM393) | ADC | `SoilHumidity.c/h` | 土壤湿度测量，支持干燥/湿润标定 |

## 驱动特点

- **非阻塞状态机**：每个传感器提供 `init` / `run` / `get` 三个标准接口
- **统一配置层**：通过 `func_config.h` 封装 HAL 库的 GPIO/I2C 宏,后续可加入其他接口进行适配
- **错误处理**：超时重试机制，连续失败后自动恢复
- **代码风格一致**：遵循 `DRIVER_NAMING_CONVENTION.md` 规范

## 项目结构

```
├── Core/                    # STM32CubeMX 生成的代码
├── Drivers/                 # HAL 库驱动
├── HardWare/
│   ├── Inc/                 # 传感器头文件
│   │   ├── func_config.h    # 配置宏封装层
│   │   ├── DHT22.h
│   │   ├── GY30.h
│   │   ├── DS18B20.h
│   │   ├── DS3231.h
│   │   ├── SoilHumidity.h
│   │   └── Sensor_Manage.h  # 传感器统一管理
│   └── Src/                 # 传感器源文件
│       ├── getTime.c        # 时间戳工具
│       └── ...
├── CMakeLists.txt
└── sensor_demo.ioc          # STM32CubeMX 配置文件
```

## 快速开始

1. 使用 STM32CubeMX 打开 `sensor_demo.ioc` 重新生成代码
2. 根据实际硬件配置 `func_config.h` 中的 GPIO/I2C/ADC 句柄
3. 编译：

```bash
mkdir build && cd build
cmake ..
make
```

4. 烧录到 STM32F429 开发板

## 硬件平台

- MCU: STM32F429
- 工具链: ARM GCC (gcc-arm-none-eabi)
- 构建系统: CMake
