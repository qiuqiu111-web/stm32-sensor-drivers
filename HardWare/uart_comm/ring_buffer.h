#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define RB_SIZE 128 // 缓冲区大小

typedef struct {
    uint8_t buffer[RB_SIZE];
    volatile uint16_t head; // 写入指针(中断里改，必须加 volatile)
    volatile uint16_t tail; // 读取指针(任务里改)
} RingBuffer_t;

void RB_Init(RingBuffer_t *rb);
bool RB_Write(RingBuffer_t *rb, uint8_t data); // 写入1字节
bool RB_Read(RingBuffer_t *rb, uint8_t *data); // 读出1字节
uint16_t RB_GetCount(RingBuffer_t *rb);        // 获取当前有多少个字节

#endif
