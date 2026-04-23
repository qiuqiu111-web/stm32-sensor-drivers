#include "Sensor_Manage.h"

// 此处添加其他传感器的头文件-----------------------------------------
#include "DS3231.h" // 时钟传感器，获取时间和温度，I2C协议
#include "DHT22.h"  // 空气温湿度传感器，获取温度和湿度，单总线协议
#include "DS18B20.h"    // 土壤温度传感器，获取土壤温度，单总线协议
#include "SoilHumidity.h" // 土壤湿度传感器，获取土壤湿度，ADC协议
#include "GY30.h" // 光照强度传感器，获取光照强度，I2C协议
#include <assert.h>
#include <stdint.h>
// -----------------------------------------------------------------


// 传感器操作函数定义-----------------------------------------
const Sensors_Ops ds3231_ops = {
    .init = (int (*)(void *))ds3231_init,
    .run = (void (*)(void *))ds3231_run,
    .if_ready = (int (*)(void *))ds3231_ready,
    .get_data = ds3231_get_adapter, // 适配器函数
};

const Sensors_Ops dht22_ops = {
    .init = (int (*)(void *))dht22_init,
    .run = (void (*)(void *))dht22_run,
    .if_ready = (int (*)(void *))dht22_ready,
    .get_data = dht22_get_adapter, // 适配器函数
};

const Sensors_Ops ds18b20_ops = {
    .init = (int (*)(void *))ds18b20_init,
    .run = (void (*)(void *))ds18b20_run,
    .if_ready = (int (*)(void *))ds18b20_ready,
    .get_data = ds18b20_get_adapter, // 适配器函数
};

const Sensors_Ops soil_humidity_ops = {
    .init = (int (*)(void *))soil_humidity_init,
    .run = (void (*)(void *))soil_humidity_run,
    .if_ready = (int (*)(void *))soil_humidity_ready,
    .get_data = soil_humidity_get_adapter, // 适配器函数
};

const Sensors_Ops gy30_ops = {
    .init = (int (*)(void *))gy30_init,
    .run = (void (*)(void *))gy30_run,
    .if_ready = (int (*)(void *))gy30_ready,
    .get_data = gy30_get_adapter, // 适配器函数
};

static int _add_sensor(Sensors_Manager *manager, eSensorsType type, void *handle) {
    if (manager->sensor_count >= MAX_SENSORS) return -1; // 超过最大传感器数量

    // 根据传感器类型设置对应的操作函数
    switch (type) {
        case SENSOR_DS3231:
            manager->sensors[manager->sensor_count].ops = (Sensor_Example) {
                .type = SENSOR_DS3231,
                .handle = (DS3231_Handle*)handle,
                .ops = &ds3231_ops,
            };
            break;
        case SENSOR_DHT22:
            manager->sensors[manager->sensor_count].ops = (Sensor_Example) {
                .type = SENSOR_DHT22,
                .handle = (DHT22_Handle*)handle,
                .ops = &dht22_ops,
            };
            break;
        case SENSOR_DS18B20:
            manager->sensors[manager->sensor_count].ops = (Sensor_Example) {
                .type = SENSOR_DS18B20,
                .handle = (DS18B20_Handle*)handle,
                .ops = &ds18b20_ops,
            };
            break;
        case SENSOR_SOIL_HUMIDITY:
            manager->sensors[manager->sensor_count].ops = (Sensor_Example) {
                .type = SENSOR_SOIL_HUMIDITY,
                .handle = (SoilHumidity_Handle*)handle,
                .ops = &soil_humidity_ops,
            };
            break;
        case SENSOR_GY30:
            manager->sensors[manager->sensor_count].ops = (Sensor_Example) {
                .type = SENSOR_GY30,
                .handle = (GY30_Handle*)handle,
                .ops = &gy30_ops,
            };
            break;
        default:
            return -1; // 不支持的传感器类型
    }
    manager->sensor_count++;

    return 0;
}

void Sensors_Manager_Init(Sensors_Manager *manager) {
    if (!manager) return -1;

    manager->sensor_count = 0;
    manager->data_flag = 0;
    manager->data_check_flag = 0;

    int i;
     // 初始化数据检查标志，每个传感器对应一个位
    for (i = 0; i < MAX_SENSORS; i++) {
        manager->data_check_flag |= (1 << i);
    }

    // 添加传感器实例到管理器（需用户根据实际情况修改）
    // 注意：添加顺序应与 main.c 中初始化顺序一致，以确保 handle 正确对应
    for (int i = 0; i < MAX_SENSORS; i++) {
        
    }

};