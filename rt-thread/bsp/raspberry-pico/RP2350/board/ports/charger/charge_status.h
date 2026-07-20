/*
 * 充电状态检测
 *
 * GPIO43：输入浮空，外部下拉；处于充电状态时为高电平。
 */
#ifndef __CHARGE_STATUS_H__
#define __CHARGE_STATUS_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHARGE_STATUS_PIN  43

void charge_status_init(void);
rt_bool_t charge_status_is_charging(void);

#ifdef __cplusplus
}
#endif

#endif /* __CHARGE_STATUS_H__ */
