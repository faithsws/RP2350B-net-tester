/*
 * 充电适配器连接检测
 *
 * GPIO40：输入浮空，外部下拉；连接充电适配器时为高电平。
 */
#ifndef __CHARGER_DETECT_H__
#define __CHARGER_DETECT_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHARGER_DETECT_PIN  40

void charger_detect_init(void);
rt_bool_t charger_detect_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* __CHARGER_DETECT_H__ */
