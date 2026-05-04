#include "cmsis_os.h"
#include "manager.h"
#include "queues.h"
#include "app_config.h"
#include <assert.h>

void GetSensorsDataTask(void const *argument) {
    Sensors_Data sensor_data;
    Sensors_Manager manager;

    // 传感器管理器初始化
    assert(Sensors_Manager_Init(&manager) == 0);

    while (1) {
        Sensors_Manager_Run(&manager);
        if (Sensors_Manager_Get_Data(&manager, &sensor_data) == 0) {
            if (xQueueSend(sensorsDataQueue, &sensor_data,
                           pdMS_TO_TICKS(100)) != pdPASS) {
                Sensors_Data discarded_data;
                xQueueReceive(sensorsDataQueue, &discarded_data, 0);
                xQueueSend(sensorsDataQueue, &sensor_data, 0);
            }
            vTaskDelay(SENSOR_SAMPLE_INTERVAL_MS);
        }
    }
}
