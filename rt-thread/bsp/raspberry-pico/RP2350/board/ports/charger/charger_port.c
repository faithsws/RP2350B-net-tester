/*
 * 充电相关 FinSH 命令
 */
#include "charger_detect.h"
#include "charge_status.h"
#include "power_status.h"

#ifdef RT_USING_FINSH
#include <finsh.h>

static void _print_line(const char *name, rt_base_t pin, rt_bool_t active, const char *active_str, const char *idle_str)
{
    rt_kprintf("  %s (GPIO%d): %s\n", name, pin, active ? active_str : idle_str);
}

static int cmd_charger(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    _print_line("Adapter", CHARGER_DETECT_PIN, charger_detect_is_connected(), "connected", "disconnected");
    return 0;
}

static int cmd_charge(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    _print_line("Charging", CHARGE_STATUS_PIN, charge_status_is_charging(), "charging", "not charging");
    return 0;
}

static int cmd_charge_info(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    /* 与 power 命令一致：适配器 + 充电 + 电池电压/档位 */
    power_status_dump();
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_charger, charger, read charger adapter status (GPIO40));
MSH_CMD_EXPORT_ALIAS(cmd_charge, charge, read charging status (GPIO43));
MSH_CMD_EXPORT_ALIAS(cmd_charge_info, charge_info, read adapter/charge/battery power status);
#endif /* RT_USING_FINSH */
