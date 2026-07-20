/*
 * 电池电压 ADC 采集（GPIO42 / ADC2）
 */
#include "battery_adc.h"
#include <hardware/adc.h>

#define BATTERY_ADC_SAMPLE_COUNT  8

static rt_bool_t battery_adc_ready = RT_FALSE;

static rt_uint32_t battery_adc_raw_to_pin_mv(rt_uint32_t raw)
{
    return (rt_uint32_t)((raw * BATTERY_ADC_VREF_MV) / BATTERY_ADC_MAX_RAW);
}

void battery_adc_init(void)
{
    adc_init();
    adc_gpio_init(BATTERY_ADC_GPIO);
    adc_select_input(BATTERY_ADC_CHANNEL);
    battery_adc_ready = RT_TRUE;
}

rt_uint16_t battery_adc_read_raw(void)
{
    rt_uint32_t sum = 0;
    rt_uint32_t i;

    if (!battery_adc_ready)
    {
        battery_adc_init();
    }

    adc_select_input(BATTERY_ADC_CHANNEL);

    /* 丢弃首样，降低通道切换误差 */
    (void)adc_read();

    for (i = 0; i < BATTERY_ADC_SAMPLE_COUNT; i++)
    {
        sum += adc_read();
    }

    return (rt_uint16_t)(sum / BATTERY_ADC_SAMPLE_COUNT);
}

rt_uint32_t battery_adc_read_pin_mv(void)
{
    return battery_adc_raw_to_pin_mv(battery_adc_read_raw());
}

rt_uint32_t battery_adc_read_mv(void)
{
    rt_uint32_t pin_mv = battery_adc_read_pin_mv();

    return (pin_mv * BATTERY_DIVIDER_NUM) / BATTERY_DIVIDER_DEN;
}

float battery_adc_read_voltage(void)
{
    return (float)battery_adc_read_mv() / 1000.0f;
}

static int battery_adc_auto_init(void)
{
    battery_adc_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(battery_adc_auto_init);

#ifdef RT_USING_FINSH
#include <finsh.h>

static int cmd_battery(int argc, char **argv)
{
    rt_uint16_t raw = battery_adc_read_raw();

    rt_kprintf("GPIO%d ADC%d raw=%u\n", BATTERY_ADC_GPIO, BATTERY_ADC_CHANNEL, raw);
    rt_kprintf("ADC pin: %u mV (%.3f V)\n",
               battery_adc_read_pin_mv(), battery_adc_read_pin_mv() / 1000.0f);
    rt_kprintf("Battery: %u mV (%.3f V)\n",
               battery_adc_read_mv(), battery_adc_read_voltage());
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_battery, battery, read battery voltage);
#endif /* RT_USING_FINSH */
