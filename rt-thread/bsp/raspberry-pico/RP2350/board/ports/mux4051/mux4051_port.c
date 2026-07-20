/*
 * TMUX4051 FinSH 调试命令
 */
#include "tx_mux4051.h"
#include "rx_mux4051.h"
#include "link_mux4051.h"

#ifdef RT_USING_FINSH
#include <finsh.h>
#include <stdlib.h>

static rt_err_t _mux_parse_channel(const char *arg, rt_uint8_t *channel)
{
    int v = atoi(arg);

    if (v < TMUX4051_CHANNEL_MIN || v > TMUX4051_CHANNEL_MAX)
    {
        return -RT_EINVAL;
    }

    *channel = (rt_uint8_t)v;
    return RT_EOK;
}

static void _print_mux(const char *name, rt_uint8_t a2, rt_uint8_t a1, rt_uint8_t a0, rt_uint8_t en,
                       rt_uint8_t channel, rt_bool_t enabled)
{
    rt_kprintf("  %s: ch=%u, %s, A2/A1/A0=GP%d/GP%d/GP%d, EN=GP%d\n",
               name, channel, enabled ? "enabled" : "disabled", a2, a1, a0, en);
}

static int cmd_tx_mux(int argc, char **argv)
{
    rt_uint8_t ch;

    if (argc < 2)
    {
        rt_kprintf("Usage: tx_mux <0-7|on|off>\n");
        return -RT_EINVAL;
    }

    if (!rt_strcmp(argv[1], "on"))
    {
        tx_mux4051_enable(RT_TRUE);
        rt_kprintf("TX mux enabled, channel=%u\n", tx_mux4051_get_channel());
        return 0;
    }

    if (!rt_strcmp(argv[1], "off"))
    {
        tx_mux4051_enable(RT_FALSE);
        rt_kprintf("TX mux disabled\n");
        return 0;
    }

    if (_mux_parse_channel(argv[1], &ch) != RT_EOK)
    {
        rt_kprintf("Invalid channel, use 0-7\n");
        return -RT_EINVAL;
    }

    tx_mux4051_select(ch);
    tx_mux4051_enable(RT_TRUE);
    rt_kprintf("TX mux channel %u selected\n", ch);
    return 0;
}

static int cmd_rx_mux(int argc, char **argv)
{
    rt_uint8_t ch;

    if (argc < 2)
    {
        rt_kprintf("Usage: rx_mux <0-7|on|off>\n");
        return -RT_EINVAL;
    }

    if (!rt_strcmp(argv[1], "on"))
    {
        rx_mux4051_enable(RT_TRUE);
        rt_kprintf("RX mux enabled, channel=%u\n", rx_mux4051_get_channel());
        return 0;
    }

    if (!rt_strcmp(argv[1], "off"))
    {
        rx_mux4051_enable(RT_FALSE);
        rt_kprintf("RX mux disabled\n");
        return 0;
    }

    if (_mux_parse_channel(argv[1], &ch) != RT_EOK)
    {
        rt_kprintf("Invalid channel, use 0-7\n");
        return -RT_EINVAL;
    }

    rx_mux4051_select(ch);
    rx_mux4051_enable(RT_TRUE);
    rt_kprintf("RX mux channel %u selected\n", ch);
    return 0;
}

static int cmd_link_mux(int argc, char **argv)
{
    rt_uint8_t ch;

    if (argc < 2)
    {
        rt_kprintf("Usage: link_mux <0-7|on|off>\n");
        return -RT_EINVAL;
    }

    if (!rt_strcmp(argv[1], "on"))
    {
        link4051_enable(RT_TRUE);
        rt_kprintf("LINK4051 enabled, channel=%u\n", link4051_get_channel());
        return 0;
    }

    if (!rt_strcmp(argv[1], "off"))
    {
        link4051_enable(RT_FALSE);
        rt_kprintf("LINK4051 disabled\n");
        return 0;
    }

    if (_mux_parse_channel(argv[1], &ch) != RT_EOK)
    {
        rt_kprintf("Invalid channel, use 0-7\n");
        return -RT_EINVAL;
    }

    link4051_select(ch);
    link4051_enable(RT_TRUE);
    rt_kprintf("LINK4051 channel %u selected\n", ch);
    return 0;
}

static int cmd_mux_info(int argc, char **argv)
{
    rt_kprintf("4051 mux status:\n");
    _print_mux("TX", TX_MUX_A2_PIN, TX_MUX_A1_PIN, TX_MUX_A0_PIN, TX_MUX_EN_PIN,
               tx_mux4051_get_channel(), tx_mux4051_is_enabled());
    rt_kprintf("    ctrl_invert=%d en_active_high=%d\n",
               TX_MUX_CTRL_INVERT, TX_MUX_EN_ACTIVE_HIGH);
    _print_mux("RX", RX_MUX_A2_PIN, RX_MUX_A1_PIN, RX_MUX_A0_PIN, RX_MUX_EN_PIN,
               rx_mux4051_get_channel(), rx_mux4051_is_enabled());
    rt_kprintf("    ctrl_invert=%d en_active_high=%d\n",
               RX_MUX_CTRL_INVERT, RX_MUX_EN_ACTIVE_HIGH);
    _print_mux("LINK4051", LINK4051_A2_PIN, LINK4051_A1_PIN, LINK4051_A0_PIN, LINK4051_EN_PIN,
               link4051_get_channel(), link4051_is_enabled());
    rt_kprintf("    ctrl_invert=%d en_active_high=%d\n",
               LINK4051_CTRL_INVERT, LINK4051_EN_ACTIVE_HIGH);
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_tx_mux, tx_mux, TX TMUX4051 channel select 0-7);
MSH_CMD_EXPORT_ALIAS(cmd_rx_mux, rx_mux, RX TMUX4051 channel select 0-7);
MSH_CMD_EXPORT_ALIAS(cmd_link_mux, link_mux, LINK4051 channel select 0-7);
MSH_CMD_EXPORT_ALIAS(cmd_mux_info, mux_info, show TX/RX/LINK4051 mux status);
#endif /* RT_USING_FINSH */
