/*
 * 发送通道 TMUX4051 实例
 */
#include "tx_mux4051.h"

static tmux4051_dev_t tx_mux_dev =
{
    .a0_pin = TX_MUX_A0_PIN,
    .a1_pin = TX_MUX_A1_PIN,
    .a2_pin = TX_MUX_A2_PIN,
    .en_pin = TX_MUX_EN_PIN,
    .en_active_high = TX_MUX_EN_ACTIVE_HIGH,
    .ctrl_invert = TX_MUX_CTRL_INVERT,
    .enabled = RT_FALSE,
    .channel = 0,
};

void tx_mux4051_init(void)
{
    tmux4051_init(&tx_mux_dev);
}

rt_err_t tx_mux4051_select(rt_uint8_t channel)
{
    return tmux4051_select(&tx_mux_dev, channel);
}

rt_err_t tx_mux4051_enable(rt_bool_t enable)
{
    return tmux4051_enable(&tx_mux_dev, enable);
}

rt_uint8_t tx_mux4051_get_channel(void)
{
    return tmux4051_get_channel(&tx_mux_dev);
}

rt_bool_t tx_mux4051_is_enabled(void)
{
    return tmux4051_is_enabled(&tx_mux_dev);
}

static int tx_mux4051_auto_init(void)
{
    tx_mux4051_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(tx_mux4051_auto_init);
