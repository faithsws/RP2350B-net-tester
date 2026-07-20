/*
 * TMUX4051 / CD4051 8 选 1 模拟开关通用驱动
 *
 * 板级有多片 4051，地址线控制方式可能不同：
 * - ctrl_invert=RT_FALSE：MCU 电平与芯片 A2/A1/A0 同相
 * - ctrl_invert=RT_TRUE ：经三极管反相，驱动时写取反码
 * EN 极性由 en_active_high 单独配置（默认低有效）。
 */
#ifndef __TMUX4051_H__
#define __TMUX4051_H__

#include <rtthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TMUX4051_CHANNEL_MIN  0
#define TMUX4051_CHANNEL_MAX  7

typedef struct
{
    rt_uint8_t a0_pin;
    rt_uint8_t a1_pin;
    rt_uint8_t a2_pin;
    rt_uint8_t en_pin;
    rt_bool_t  en_active_high; /* RT_TRUE：EN 高电平使能 */
    rt_bool_t  ctrl_invert;    /* RT_TRUE：地址线反相控制（写取反码） */
    rt_bool_t  enabled;
    rt_uint8_t channel;
} tmux4051_dev_t;

void tmux4051_init(tmux4051_dev_t *dev);
rt_err_t tmux4051_select(tmux4051_dev_t *dev, rt_uint8_t channel);
rt_err_t tmux4051_enable(tmux4051_dev_t *dev, rt_bool_t enable);
rt_uint8_t tmux4051_get_channel(const tmux4051_dev_t *dev);
rt_bool_t tmux4051_is_enabled(const tmux4051_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* __TMUX4051_H__ */
