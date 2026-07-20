/*
 * EC11 旋转编码器驱动
 *
 * A: GPIO33, B: GPIO34, KEY: GPIO35
 * 内部上拉，KEY 低电平有效
 */
#ifndef __EC11_H__
#define __EC11_H__

#include <rtthread.h>

#define EC11_PIN_A   33
#define EC11_PIN_B   34
#define EC11_PIN_KEY 35

void ec11_init(void);
/* 兼容旧接口：A/B 已改中断采集，此函数为空操作 */
void ec11_poll(void);
int16_t ec11_take_diff(void);
rt_bool_t ec11_key_pressed(void);

#endif /* __EC11_H__ */
