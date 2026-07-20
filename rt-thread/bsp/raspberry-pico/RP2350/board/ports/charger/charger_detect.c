/*
 * 充电适配器连接检测（GPIO40）
 */
#include "charger_detect.h"
#include <hardware/gpio.h>

static rt_bool_t charger_detect_ready = RT_FALSE;

void charger_detect_init(void)
{
    gpio_init(CHARGER_DETECT_PIN);
    gpio_set_dir(CHARGER_DETECT_PIN, GPIO_IN);
    gpio_disable_pulls(CHARGER_DETECT_PIN);
    charger_detect_ready = RT_TRUE;
}

rt_bool_t charger_detect_is_connected(void)
{
    if (!charger_detect_ready)
    {
        charger_detect_init();
    }

    return gpio_get(CHARGER_DETECT_PIN) ? RT_TRUE : RT_FALSE;
}

static int charger_detect_auto_init(void)
{
    charger_detect_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(charger_detect_auto_init);
