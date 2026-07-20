/*
 * CTRL_H 74HC238 实例
 */
#include "ctrl_h_hc238.h"

static hc238_dev_t ctrl_h_dev =
{
    .a0_pin = CTRL_H_A0_PIN,
    .a1_pin = CTRL_H_A1_PIN,
    .a2_pin = CTRL_H_A2_PIN,
    .channel = 0,
};

void ctrl_h_hc238_init(void)
{
    hc238_init(&ctrl_h_dev);
}

rt_err_t ctrl_h_hc238_select(rt_uint8_t channel)
{
    return hc238_select(&ctrl_h_dev, channel);
}

rt_uint8_t ctrl_h_hc238_get_channel(void)
{
    return hc238_get_channel(&ctrl_h_dev);
}
