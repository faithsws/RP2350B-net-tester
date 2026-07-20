/*
 * 外部升压电源使能（GPIO32）
 */
#include "boost_pwr.h"
#include <hardware/gpio.h>

static rt_bool_t boost_pwr_ready = RT_FALSE;
static rt_bool_t boost_pwr_enabled = RT_FALSE;

void boost_pwr_init(void)
{
    gpio_init(BOOST_PWR_EN_PIN);
    gpio_set_dir(BOOST_PWR_EN_PIN, GPIO_OUT);
    gpio_put(BOOST_PWR_EN_PIN, 0);
    boost_pwr_ready = RT_TRUE;
    boost_pwr_enabled = RT_FALSE;
}

void boost_pwr_on(void)
{
    if (!boost_pwr_ready)
    {
        boost_pwr_init();
    }

    gpio_put(BOOST_PWR_EN_PIN, 1);
    boost_pwr_enabled = RT_TRUE;
}

void boost_pwr_off(void)
{
    if (!boost_pwr_ready)
    {
        boost_pwr_init();
        return;
    }

    gpio_put(BOOST_PWR_EN_PIN, 0);
    boost_pwr_enabled = RT_FALSE;
}

rt_bool_t boost_pwr_is_enabled(void)
{
    if (!boost_pwr_ready)
    {
        return RT_FALSE;
    }

    return boost_pwr_enabled;
}

static int boost_pwr_auto_init(void)
{
    boost_pwr_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(boost_pwr_auto_init);

#ifdef RT_USING_FINSH
#include <finsh.h>

static int cmd_boost_pwr(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: boost_pwr <on|off|status>\n");
        return -RT_EINVAL;
    }

    if (!rt_strcmp(argv[1], "on"))
    {
        boost_pwr_on();
        rt_kprintf("Boost power enabled (GPIO%d=HIGH)\n", BOOST_PWR_EN_PIN);
        return 0;
    }

    if (!rt_strcmp(argv[1], "off"))
    {
        boost_pwr_off();
        rt_kprintf("Boost power disabled (GPIO%d=LOW)\n", BOOST_PWR_EN_PIN);
        return 0;
    }

    if (!rt_strcmp(argv[1], "status"))
    {
        rt_kprintf("Boost power (GPIO%d): %s\n",
                   BOOST_PWR_EN_PIN,
                   boost_pwr_is_enabled() ? "on" : "off");
        return 0;
    }

    rt_kprintf("Unknown option, use on/off/status\n");
    return -RT_EINVAL;
}
MSH_CMD_EXPORT_ALIAS(cmd_boost_pwr, boost_pwr, external boost power enable);
#endif /* RT_USING_FINSH */
