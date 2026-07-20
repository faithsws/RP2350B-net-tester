/*
 * 供电状态汇总与 FinSH 测试命令
 */
#include "power_status.h"
#include "battery_adc.h"
#include "charger_detect.h"
#include "charge_status.h"

rt_uint8_t power_status_level_from_mv(rt_uint32_t mv)
{
    if (mv >= POWER_BAT_MV_LVL4)
        return 4;
    if (mv >= POWER_BAT_MV_LVL3)
        return 3;
    if (mv >= POWER_BAT_MV_LVL2)
        return 2;
    return 1;
}

void power_status_get(power_status_t *st)
{
    if (!st)
        return;

    st->adapter = charger_detect_is_connected();
    st->charging = charge_status_is_charging();
    st->voltage_mv = battery_adc_read_mv();
    st->level = power_status_level_from_mv(st->voltage_mv);
    st->low = (st->voltage_mv < POWER_BAT_MV_LOW) ? RT_TRUE : RT_FALSE;
}

void power_status_dump(void)
{
    power_status_t st;
    rt_uint16_t raw;

    power_status_get(&st);
    raw = battery_adc_read_raw();

    rt_kprintf("Power status:\n");
    rt_kprintf("  Adapter  (GPIO%d): %s\n",
               CHARGER_DETECT_PIN,
               st.adapter ? "inserted (HIGH)" : "removed (LOW)");
    rt_kprintf("  Charging (GPIO%d): %s\n",
               CHARGE_STATUS_PIN,
               st.charging ? "charging (HIGH)" : "idle (LOW)");
    rt_kprintf("  Battery  (GPIO%d ADC%d):\n",
               BATTERY_ADC_GPIO, BATTERY_ADC_CHANNEL);
    rt_kprintf("    raw=%u  pin=%u mV  Vbat=%u mV (%.3f V)\n",
               raw,
               battery_adc_read_pin_mv(),
               st.voltage_mv,
               st.voltage_mv / 1000.0f);
    rt_kprintf("    level=%u/%u  %s\n",
               st.level, POWER_BAT_LEVEL_MAX,
               st.low ? "LOW" : "OK");
}

#ifdef RT_USING_FINSH
#include <finsh.h>

static int cmd_power(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    power_status_dump();
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_power, power, dump adapter/charge/battery power status);
#endif /* RT_USING_FINSH */
