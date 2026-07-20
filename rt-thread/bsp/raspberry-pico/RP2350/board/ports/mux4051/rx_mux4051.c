/*
 * 接收通道 TMUX4051 实例
 */
#include "rx_mux4051.h"

static tmux4051_dev_t rx_mux_dev =
{
    .a0_pin = RX_MUX_A0_PIN,
    .a1_pin = RX_MUX_A1_PIN,
    .a2_pin = RX_MUX_A2_PIN,
    .en_pin = RX_MUX_EN_PIN,
    .en_active_high = RX_MUX_EN_ACTIVE_HIGH,
    .ctrl_invert = RX_MUX_CTRL_INVERT,
    .enabled = RT_FALSE,
    .channel = 0,
};

void rx_mux4051_init(void)
{
    tmux4051_init(&rx_mux_dev);
}

rt_err_t rx_mux4051_select(rt_uint8_t channel)
{
    return tmux4051_select(&rx_mux_dev, channel);
}

rt_err_t rx_mux4051_enable(rt_bool_t enable)
{
    return tmux4051_enable(&rx_mux_dev, enable);
}

rt_uint8_t rx_mux4051_get_channel(void)
{
    return tmux4051_get_channel(&rx_mux_dev);
}

rt_bool_t rx_mux4051_is_enabled(void)
{
    return tmux4051_is_enabled(&rx_mux_dev);
}

static int rx_mux4051_auto_init(void)
{
    rx_mux4051_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rx_mux4051_auto_init);
