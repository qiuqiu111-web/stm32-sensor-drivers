#ifndef GETTIME_H
#define GETTIME_H

// 获取当前时间戳（微秒）
uint32_t get_current_timestamp_us(void) ;
// 获取当前时间戳（毫秒）
uint32_t get_current_timestamp_ms(void) ;

// 延时微秒
void my_delay_us(uint32_t us) ;

#endif // GETTIME_H