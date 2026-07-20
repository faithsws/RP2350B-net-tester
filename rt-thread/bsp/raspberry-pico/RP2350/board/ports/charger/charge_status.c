/*
 * 充电状态检测（GPIO43）
 */
#include "charge_status.h"
#include <hardware/gpio.h>

static rt_bool_t charge_status_ready = RT_FALSE;

void charge_status_init(void)
{
    gpio_init(CHARGE_STATUS_PIN);
    gpio_set_dir(CHARGE_STATUS_PIN, GPIO_IN);
    gpio_disable_pulls(CHARGE_STATUS_PIN);
    charge_status_ready = RT_TRUE;
}

rt_bool_t charge_status_is_charging(void)
{
    if (!charge_status_ready)
    {
        charge_status_init();
    }

    return gpio_get(CHARGE_STATUS_PIN) ? RT_TRUE : RT_FALSE;
}

static int charge_status_auto_init(void)
{
    charge_status_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(charge_status_auto_init);
