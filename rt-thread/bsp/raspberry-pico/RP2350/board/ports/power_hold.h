/*
 * 电源保持引脚驱动
 *
 * GPIO37 控制外部电源锁存电路，上电后拉高以保持供电，
 * 关机时拉低以释放电源。
 */
#ifndef __POWER_HOLD_H__
#define __POWER_HOLD_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POWER_HOLD_PIN 37

void power_hold_init(void);
void power_hold_on(void);
void power_hold_off(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_HOLD_H__ */
