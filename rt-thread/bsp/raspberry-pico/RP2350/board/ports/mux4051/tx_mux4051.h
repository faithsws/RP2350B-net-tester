/*
 * 发送通道 TMUX4051
 *
 * A2=GPIO14, A1=GPIO15, A0=GPIO16, EN=GPIO18（低有效）
 * 地址线同相 → ctrl_invert=RT_FALSE
 */
#ifndef __TX_MUX4051_H__
#define __TX_MUX4051_H__

#include "tmux4051.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TX_MUX_A2_PIN         14
#define TX_MUX_A1_PIN         15
#define TX_MUX_A0_PIN         16
#define TX_MUX_EN_PIN         18
#define TX_MUX_CTRL_INVERT    RT_FALSE  /* 非反相控制 */
#define TX_MUX_EN_ACTIVE_HIGH RT_FALSE

void tx_mux4051_init(void);
rt_err_t tx_mux4051_select(rt_uint8_t channel);
rt_err_t tx_mux4051_enable(rt_bool_t enable);
rt_uint8_t tx_mux4051_get_channel(void);
rt_bool_t tx_mux4051_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __TX_MUX4051_H__ */
