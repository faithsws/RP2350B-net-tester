/*
 * 74HC238 3-8 译码器通用驱动
 *
 * 使能后，选中通道 Yn 输出高电平（打开对应 PMOS/NMOS），其余 Y 为低。
 * 未使能时全部 Y 输出低（MOS 全关）。
 *
 * 使能接法：G0/G1 硬件下拉到 GND，GPIO24 接 G2，高电平使能。
 * CTRL_H（PMOS）与 CTRL_L（NMOS）共用 EN，禁止同通道同时选中（会短路）。
 */
#ifndef __HC238_H__
#define __HC238_H__

#include <rtthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HC238_CHANNEL_MIN  0
#define HC238_CHANNEL_MAX  7

/* 两片 238 共用：GPIO24 -> G2，高电平使能（G0/G1 硬件接低） */
#define HC238_EN_PIN           24
#define HC238_EN_ACTIVE_LEVEL  1

typedef struct
{
    rt_uint8_t a0_pin;
    rt_uint8_t a1_pin;
    rt_uint8_t a2_pin;
    rt_uint8_t channel;
} hc238_dev_t;

void hc238_bus_init(void);
rt_err_t hc238_bus_enable(rt_bool_t enable);
rt_bool_t hc238_bus_is_enabled(void);

void hc238_init(hc238_dev_t *dev);
rt_err_t hc238_select(hc238_dev_t *dev, rt_uint8_t channel);
rt_uint8_t hc238_get_channel(const hc238_dev_t *dev);

/**
 * H 桥安全导通：先关 EN，再设 PMOS/NMOS 通道，最后开 EN。
 * @param pmos_ch / nmos_ch 通道 0-7（对应线号 1-8）
 * @return -RT_EINVAL 同通道或越界；否则 RT_EOK
 */
rt_err_t hc238_hbridge_on(rt_uint8_t pmos_ch, rt_uint8_t nmos_ch);

/* 关闭 H 桥（拉低 EN，两片 Yn 全低） */
void hc238_hbridge_off(void);

#ifdef __cplusplus
}
#endif

#endif /* __HC238_H__ */
