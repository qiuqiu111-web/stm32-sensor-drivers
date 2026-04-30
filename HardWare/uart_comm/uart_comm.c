#include <stdint.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "uart_comm.h"

// 协议 [帧头 2字节] [数据 22字节] [校验 1字节] [帧尾 2字节]
// 共计 27 字节，其中数据部分为 Sensors_Data_Wire 结构体，长度固定为 22 字节

// 串口发送的结构体，进行字节压缩
#pragma pack(push, 1)
typedef struct {
    float temperature; 
    float humidity;    
    float soil_humidity;
    float illuminance; 
    uint8_t seconds;   
    uint8_t minutes;   
    uint8_t hours;     
    uint8_t date;      
    uint8_t month;     
    uint8_t year;      
} Sensors_Data_Wire;   
#pragma pack(pop)

static uint8_t tx_buf[UART_FRAME_SIZE];  // 发送缓冲区

static uint8_t _process_data(Sensors_Data* data, Sensors_Data_Wire* wire_data) {
    // 将 Sensors_Data 结构体的数据转换为适合传输的 Sensors_Data_Wire 结构体
    wire_data->temperature = data->temperature;
    wire_data->humidity = data->humidity;
    wire_data->soil_humidity = data->soil_humidity;
    wire_data->illuminance = data->illuminance;
    wire_data->seconds = data->seconds;
    wire_data->minutes = data->minutes;
    wire_data->hours = data->hours;
    wire_data->date = data->date;
    wire_data->month = data->month;
    wire_data->year = data->year;

    return 0;
}

static uint8_t _compute_checksum(uint8_t* data, uint8_t len) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

// int UART_Comm_Init(UART_HandleTypeDef *huart) {

    // 这里假设 huart 已经在 main.c 中定义并初始化了

    // if (!huart) {
    //     return -1; // 检查指针是否为空
    // }

    // huart->Instance = USART1;
    // huart->Init.BaudRate = 115200;
    // huart->Init.WordLength = UART_WORDLENGTH_8B;
    // huart->Init.StopBits = UART_STOPBITS_1;
    // huart->Init.Parity = UART_PARITY_NONE;
    // huart->Init.Mode = UART_MODE_TX_RX;
    // huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    // huart->Init.OverSampling = UART_OVERSAMPLING_16;

    // if (HAL_UART_Init(huart) != HAL_OK) {
    //     return -1;
    // }
    // return 0;
// }

int UART_Comm_Send(Sensors_Data* data, UART_HandleTypeDef *huart) {
    if (!data) return -1;

    Sensors_Data_Wire wire_data;
    uint8_t index = 0;
    uint8_t retry_count = 0;

    // 1. 转换数据结构
    _process_data(data, &wire_data);

    // 2. 帧头
    tx_buf[index++] = UART_HEADER_1;
    tx_buf[index++] = UART_HEADER_2;

    // 3. 直接拷贝整个结构体
    memcpy(&tx_buf[index], &wire_data, UART_DATA_SIZE);
    index += UART_DATA_SIZE;

    // 4. 计算并追加校验和（只校验数据部分）
    tx_buf[index++] = _compute_checksum(&tx_buf[2], UART_DATA_SIZE);

    // 5. 帧尾
    tx_buf[index++] = UART_TAIL_1;
    tx_buf[index++] = UART_TAIL_2;

    // 发送数据
    while (HAL_UART_Transmit(huart, tx_buf, UART_FRAME_SIZE, 100) != HAL_OK) {
        retry_count++;
        if (retry_count >= MAX_RETRY_COUNT) {
            return -1;
        }
    }
    return 0;
}
