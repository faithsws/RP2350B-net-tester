/*
 * 74HC238 通用驱动（共用 EN）
 */
#include "hc238.h"
#include <hardware/gpio.h>

static rt_bool_t hc238_bus_enabled = RT_FALSE;

static void hc238_gpio_out(rt_uint8_t pin, rt_bool_t level)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, level ? 1 : 0);
}

static void hc238_apply_en(rt_bool_t enable)
{
#if HC238_EN_ACTIVE_LEVEL
    gpio_put(HC238_EN_PIN, enable ? 1 : 0);
#else
    gpio_put(HC238_EN_PIN, enable ? 0 : 1);
#endif
}

static void hc238_apply_channel(const hc238_dev_t *dev)
{
    rt_uint8_t ch = dev->channel;

    gpio_put(dev->a0_pin, (ch >> 0) & 0x01);
    gpio_put(dev->a1_pin, (ch >> 1) & 0x01);
    gpio_put(dev->a2_pin, (ch >> 2) & 0x01);
}

void hc238_bus_init(void)
{
    hc238_gpio_out(HC238_EN_PIN, HC238_EN_ACTIVE_LEVEL ? 0 : 1);
    hc238_bus_enabled = RT_FALSE;
}

rt_err_t hc238_bus_enable(rt_bool_t enable)
{
    hc238_bus_enabled = enable;
    hc238_apply_en(enable);
    return RT_EOK;
}

rt_bool_t hc238_bus_is_enabled(void)
{
    return hc238_bus_enabled;
}

void hc238_init(hc238_dev_t *dev)
{
    hc238_gpio_out(dev->a0_pin, 0);
    hc238_gpio_out(dev->a1_pin, 0);
    hc238_gpio_out(dev->a2_pin, 0);
    dev->channel = 0;
}

rt_err_t hc238_select(hc238_dev_t *dev, rt_uint8_t channel)
{
    if (channel > HC238_CHANNEL_MAX)
    {
        return -RT_EINVAL;
    }

    dev->channel = channel;
    hc238_apply_channel(dev);
    return RT_EOK;
}

rt_uint8_t hc238_get_channel(const hc238_dev_t *dev)
{
    return dev->channel;
}
