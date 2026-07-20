/*
 * 接收通道 TMUX4051
 *
 * A2=GPIO7, A1=GPIO6, A0=GPIO5, EN=GPIO8（低有效）
 * 地址线同相 → ctrl_invert=RT_FALSE
 */
#ifndef __RX_MUX4051_H__
#define __RX_MUX4051_H__

#include "tmux4051.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RX_MUX_A2_PIN         7
#define RX_MUX_A1_PIN         6
#define RX_MUX_A0_PIN         5
#define RX_MUX_EN_PIN         8
#define RX_MUX_CTRL_INVERT    RT_FALSE  /* 非反相控制 */
#define RX_MUX_EN_ACTIVE_HIGH RT_FALSE

void rx_mux4051_init(void);
rt_err_t rx_mux4051_select(rt_uint8_t channel);
rt_err_t rx_mux4051_enable(rt_bool_t enable);
rt_uint8_t rx_mux4051_get_channel(void);
rt_bool_t rx_mux4051_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __RX_MUX4051_H__ */
