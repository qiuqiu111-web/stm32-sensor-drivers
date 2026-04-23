#ifndef __SENSOR_MANAGE_H
#define __SENSOR_MANAGE_H

#define MAX_SENSORS 5

// 特殊数据结构体声明
#include "DS3231.h"

typedef enum {
    SENSOR_DS3231,
    SENSOR_DHT22,
    SENSOR_DS18B20,
    SENSOR_SOIL_HUMIDITY,
    SENSOR_GY30
} eSensorsType;

typedef struct {
    float temperature; // 温度
    float humidity;    // 湿度
    float soil_humidity; // 土壤湿度
    float illuminance; // 光照强度
    DS3231_Time time;  // 时间数据
} Sensors_Data;

typedef struct {
    int (*init)(void *handle); // 初始化函数指针
    void (*run)(void *handle);  // 状态机驱动函数指针
    int (*if_ready)(void *handle); // 数据就绪检查函数指针，返回0表示数据准备好
    int (*get_data)(void *handle, Sensors_Data *data); // 获取数据函数指针，data为输出参数
} Sensors_Ops;

typedef struct {
    eSensorsType type; // 传感器类型
    void *handle;     // 传感器句柄指针 
    const Sensors_Ops *ops;   // 传感器操作函数指针
} Sensor_Example;

typedef struct {
    int sensor_count; // 传感器数量
    Sensor_Example sensors[MAX_SENSORS]; // 传感器数组，最多支持5个传感器
    int data_check_flag; // 数据检查标志，当数据标志位等于传感器数量时表示所有传感器数据都准备好了
    int data_flag; // 数据标志，表示是否有新数据准备好
} Sensors_Manager;

// 传感器管理器接口
int Sensors_Manager_Init(Sensors_Manager *manager);
// 运行状态机，非阻塞，需定期调用
void Sensors_Manager_Run(Sensors_Manager *manager);
// 检查是否有数据准备好，返回0表示有数据
int Sensors_Ready(Sensors_Manager *manager); // 检查是否有数据准备好，返回0表示有数据
// 获取已准备好的数据，返回0表示成功，非0表示无数据或错误
int Sensors_Manager_Get_Data(Sensors_Manager *manager, Sensors_Data *data);

#endif // __SENSOR_MANAGE_H