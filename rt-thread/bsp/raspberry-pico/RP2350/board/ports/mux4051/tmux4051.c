/*
 * TMUX4051 / CD4051 通用驱动
 */
#include "tmux4051.h"
#include <hardware/gpio.h>

static void tmux4051_gpio_out(rt_uint8_t pin, rt_bool_t level)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, level ? 1 : 0);
}

static void tmux4051_apply_channel(const tmux4051_dev_t *dev)
{
    rt_uint8_t ch = dev->channel;

    /* 反相控制：MCU 输出取反码，抵消三极管反相 */
    if (dev->ctrl_invert)
    {
        ch = (rt_uint8_t)((~ch) & 0x07u);
    }

    gpio_put(dev->a0_pin, (ch >> 0) & 0x01);
    gpio_put(dev->a1_pin, (ch >> 1) & 0x01);
    gpio_put(dev->a2_pin, (ch >> 2) & 0x01);
}

static void tmux4051_apply_enable(const tmux4051_dev_t *dev)
{
    rt_bool_t level;

    if (dev->en_active_high)
    {
        level = dev->enabled ? RT_TRUE : RT_FALSE;
    }
    else
    {
        /* 默认低有效：使能时拉低 */
        level = dev->enabled ? RT_FALSE : RT_TRUE;
    }

    gpio_put(dev->en_pin, level ? 1 : 0);
}

void tmux4051_init(tmux4051_dev_t *dev)
{
    rt_uint8_t a0 = 0;
    rt_uint8_t a1 = 0;
    rt_uint8_t a2 = 0;

    if (dev->ctrl_invert)
    {
        /* 逻辑通道 0 → 硬件码 7 */
        a0 = 1;
        a1 = 1;
        a2 = 1;
    }

    tmux4051_gpio_out(dev->a0_pin, a0);
    tmux4051_gpio_out(dev->a1_pin, a1);
    tmux4051_gpio_out(dev->a2_pin, a2);

    /* 默认关闭 */
    tmux4051_gpio_out(dev->en_pin, dev->en_active_high ? 0 : 1);

    dev->channel = 0;
    dev->enabled = RT_FALSE;
}

rt_err_t tmux4051_select(tmux4051_dev_t *dev, rt_uint8_t channel)
{
    if (channel > TMUX4051_CHANNEL_MAX)
    {
        return -RT_EINVAL;
    }

    dev->channel = channel;
    tmux4051_apply_channel(dev);
    return RT_EOK;
}

rt_err_t tmux4051_enable(tmux4051_dev_t *dev, rt_bool_t enable)
{
    dev->enabled = enable;
    tmux4051_apply_enable(dev);
    return RT_EOK;
}

rt_uint8_t tmux4051_get_channel(const tmux4051_dev_t *dev)
{
    return dev->channel;
}

rt_bool_t tmux4051_is_enabled(const tmux4051_dev_t *dev)
{
    return dev->enabled;
}
