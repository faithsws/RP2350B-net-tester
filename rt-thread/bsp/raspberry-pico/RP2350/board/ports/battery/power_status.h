/*
 * 供电状态汇总：适配器 / 充电 / 电池电量档位
 */
#ifndef __POWER_STATUS_H__
#define __POWER_STATUS_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 锂电池电压档位阈值（mV），可按电芯特性微调 */
#define POWER_BAT_MV_LVL4   4100  /* 满电 */
#define POWER_BAT_MV_LVL3   3900
#define POWER_BAT_MV_LVL2   3700
#define POWER_BAT_MV_LOW    3500  /* 低于此为低电警告 */

/* 电量档位：1~4 */
#define POWER_BAT_LEVEL_MIN 1
#define POWER_BAT_LEVEL_MAX 4

typedef struct
{
    rt_bool_t   adapter;      /* GPIO40：适配器已插入 */
    rt_bool_t   charging;     /* GPIO43：正在充电 */
    rt_uint32_t voltage_mv;   /* GPIO42 ADC：电池电压 mV */
    rt_uint8_t  level;        /* 1~4 档 */
    rt_bool_t   low;          /* 低电量 */
} power_status_t;

void power_status_get(power_status_t *st);
rt_uint8_t power_status_level_from_mv(rt_uint32_t mv);
void power_status_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* __POWER_STATUS_H__ */
