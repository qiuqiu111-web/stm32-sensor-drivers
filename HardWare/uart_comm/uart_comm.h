#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>
#include "usart.h"
#include "Sensor_Manage.h" // 包含传感器数据结构定义

#define UART_DATA_SIZE 22  // 定义数据部分的大小（Sensors_Data_Wire结构体大小）
#define UART_FRAME_SIZE UART_DATA_SIZE + 5 // 协议帧总大小 = 数据部分 + 帧头(2字节) + 校验(1字节) + 帧尾(2字节)

// 帧头和帧尾定义
#define UART_HEADER_1 0xAA
#define UART_HEADER_2 0x55
#define UART_TAIL_1 0x55
#define UART_TAIL_2 0xAA

#define MAX_RETRY_COUNT 3 // 定义最大重试次数

// int UART_Comm_Init(UART_HandleTypeDef *huart);
int UART_Comm_Send(Sensors_Data* data, UART_HandleTypeDef *huart);

#endif // UART_COMM_H