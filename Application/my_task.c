#include "Sensor_Manage.h"
#include "uart_comm.h"
#include "cmsis_os.h"

// 传感器数据队列定义
Sensors_Data sensorsDataQueue[10];
StaticQueue_t xSensorsDataQueueBuffer;
uint8_t pucSensorsDataQueueStorage[10 * sizeof(Sensors_Data)];


int QueueInit(void) {
    sensorsDataQueue = xQueueCreateStatic(10, sizeof(Sensors_Data), pucSensorsDataQueueStorage, &xSensorsDataQueueBuffer);
    if (sensorsDataQueue == NULL) {
        return -1; // 创建队列失败
    }
    return 0; // 创建队列成功
}

// 获取传感器数据的任务函数
void GetSensorsDataTask(void const * argument) {
    Sensors_Data sensor_data; 
    Sensors_Manager manager;
    if (Sensors_Manager_Init(&manager) != 0) {
        // 处理初始化错误
        Error_Handler();
    }

    while (1) {
        Sensors_Manager_Run(&manager);
        if (Sensors_Manager_Get_Data(&manager, &sensor_data) == 0) {
            if (xQueueSend(sensorsDataQueue, &sensor_data, pdMS_TO_TICKS(100)) != pdPASS) {
            // 队列满了，移除最旧的数据
            Sensors_Data discarded_data;
            xQueueReceive(sensorsDataQueue, &discarded_data, 0);

            // 再次尝试发送新数据
            xQueueSend(sensorsDataQueue, &sensor_data, 0);
            }
            vTaskDelay(2000); // 延时2秒
        }
    }
}

// 处理并发送数据的任务函数
void SendDataTask(void const * argument) {
    Sensors_Data sensor_data;
    while (1) {
        // 从队列获取数据
        if (xQueueReceive(sensorsDataQueue, &sensor_data, portMAX_DELAY) == pdPASS) {
            // 发送数据给上位机
            UART_Comm_Send(&sensor_data, &huart6);
        }
    }
}

// 接受并处理上位机命令的任务函数
void ReceiveCommandTask(void const * argument) {

    while (1) {
        // 接收上位机命令并处理
        
    }
}

// 执行控制命令的任务函数
void ControlCommandTask(void const * argument) {

    while (1) {
        // 从队列获取控制命令并执行
        
    }
}