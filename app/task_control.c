#include "cmsis_os.h"
#include "queues.h"
#include "pump_driver.h"

void ControlCommandTask(void const *argument) {
    UART_Command command;
    pump_dev_t *water_pump = Pump_Create(&htim3, TIM_CHANNEL_1);

    while (1) {
        if (xQueueReceive(commandQueue, &command, portMAX_DELAY) == pdPASS) {
            if (command.command_id == 0x01) {
                uint8_t speed_percent = command.payload;
                Pump_SetSpeed(water_pump, speed_percent);
            }
        }
    }
}
