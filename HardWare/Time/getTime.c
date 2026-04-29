#include "func_config.h"

#define F_CPU SystemCoreClock // 定义CPU频率（Hz），根据实际情况修改
#define CYCNT  DWT->CYCCNT    // DWT周期计数器寄存器地址

uint32_t get_current_timestamp_us(void) {
    return (uint32_t)(((uint64_t)CYCNT * 1000000ULL) / F_CPU);
}
uint32_t get_current_timestamp_ms(void) {
    return (uint32_t)(((uint64_t)CYCNT * 1000ULL) / F_CPU);
}

// 延时微秒（忙等待）
// void my_delay_us(uint32_t us) {
//     uint32_t start = get_current_timestamp_us();
//     while (get_current_timestamp_us() - start < us) {
//     }
// }

void my_delay_us(uint32_t us) {
    uint32_t start = CYCNT;
    uint32_t ticks = (uint32_t)(((uint64_t)F_CPU * us) / 1000000ULL);
    while ((CYCNT - start) < ticks) { __NOP(); }
}