#include "cmsis_os.h"
#include "uart_hal.h"
#include "queues.h"

void ReceiveCommandTask(void const *argument) {
    UART_Comm_StartReceive(&huart6, xTaskGetCurrentTaskHandle());

    UART_Command command;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (UART_Comm_Receive(&command, &huart6) == 0) {
            xQueueSend(commandQueue, &command, 0);
        }
    }
}
