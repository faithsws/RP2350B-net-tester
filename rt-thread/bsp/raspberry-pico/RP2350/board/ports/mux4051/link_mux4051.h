/*
 * LINK4051（CD4051）模拟开关：将对线通路电压路由到 ADC
 *
 * A2=GPIO30, A1=GPIO29, A0=GPIO28
 * EN=GPIO31，高电平使能
 * 地址线经三极管反相 → ctrl_invert=RT_TRUE
 */
#ifndef __LINK_MUX4051_H__
#define __LINK_MUX4051_H__

#include "tmux4051.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LINK4051_A2_PIN       30
#define LINK4051_A1_PIN       29
#define LINK4051_A0_PIN       28
#define LINK4051_EN_PIN       31
#define LINK4051_CTRL_INVERT  RT_TRUE   /* 反相控制 */
#define LINK4051_EN_ACTIVE_HIGH RT_TRUE

void link4051_init(void);
rt_err_t link4051_select(rt_uint8_t channel);
rt_err_t link4051_enable(rt_bool_t enable);
rt_uint8_t link4051_get_channel(void);
rt_bool_t link4051_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __LINK_MUX4051_H__ */
