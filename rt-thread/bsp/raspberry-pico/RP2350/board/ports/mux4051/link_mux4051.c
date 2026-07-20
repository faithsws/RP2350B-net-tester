/*
 * LINK4051（CD4051）实例：反相控制 + EN 高有效
 */
#include "link_mux4051.h"

static tmux4051_dev_t link4051_dev =
{
    .a0_pin = LINK4051_A0_PIN,
    .a1_pin = LINK4051_A1_PIN,
    .a2_pin = LINK4051_A2_PIN,
    .en_pin = LINK4051_EN_PIN,
    .en_active_high = LINK4051_EN_ACTIVE_HIGH,
    .ctrl_invert = LINK4051_CTRL_INVERT,
    .enabled = RT_FALSE,
    .channel = 0,
};

void link4051_init(void)
{
    tmux4051_init(&link4051_dev);
}

rt_err_t link4051_select(rt_uint8_t channel)
{
    return tmux4051_select(&link4051_dev, channel);
}

rt_err_t link4051_enable(rt_bool_t enable)
{
    return tmux4051_enable(&link4051_dev, enable);
}

rt_uint8_t link4051_get_channel(void)
{
    return tmux4051_get_channel(&link4051_dev);
}

rt_bool_t link4051_is_enabled(void)
{
    return tmux4051_is_enabled(&link4051_dev);
}

static int link4051_auto_init(void)
{
    link4051_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(link4051_auto_init);
