#include "ring_buffer.h"

void RB_Init(RingBuffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

bool RB_Write(RingBuffer_t *rb, uint8_t data) {
    uint16_t next = (rb->head + 1) % RB_SIZE;
    if (next == rb->tail) return false; // 满了
    rb->buffer[rb->head] = data;
    rb->head = next;
    return true;
}

bool RB_Read(RingBuffer_t *rb, uint8_t *data) {
    if (rb->head == rb->tail) return false; // 空了
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % RB_SIZE;
    return true;
}

uint16_t RB_GetCount(RingBuffer_t *rb) {
    return (rb->head - rb->tail + RB_SIZE) % RB_SIZE;
}
