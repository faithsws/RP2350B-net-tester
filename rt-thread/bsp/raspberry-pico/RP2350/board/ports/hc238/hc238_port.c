/*
 * 74HC238 FinSH 调试命令 + H 桥扫描（仅采样）
 *
 * 扫描前打开升压；56 步各开一对 PMOS/NMOS，
 * 每步对 CD4051 通道 0~7 依次采样，电压写入 V[H][L][MUX]；
 * 扫描结束后回调，由上层调用 pair_judge / seq_judge。
 */
#include "ctrl_h_hc238.h"
#include "ctrl_l_hc238.h"
#include "boost_pwr.h"
#include "link_mux4051.h"
#include "link_adc.h"
#include "pair_scan.h"

#include <string.h>

#define PAIR_SCAN_BOOST_MS      50
#define PAIR_SCAN_HL_SETTLE_MS  20
#define PAIR_SCAN_MUX_SETTLE_MS 20
#define PAIR_SCAN_HOLD_MS       10
#define PAIR_SCAN_STACK         3072
#define PAIR_SCAN_PRIO          (RT_THREAD_PRIORITY_MAX / 2 + 2)
#define PAIR_SCAN_TOTAL         (8 * 7)

static volatile rt_bool_t pair_scan_running = RT_FALSE;
static volatile rt_bool_t pair_scan_abort = RT_FALSE;
static rt_thread_t pair_scan_tid = RT_NULL;

static pair_scan_done_cb_t pair_scan_done_cb = RT_NULL;
static void *pair_scan_done_user = RT_NULL;

/* 三维电压：真实电压 mV（已 ×3）；H==L 为 INVALID */
static pair_volt_cube_t s_volt_mv;

rt_err_t hc238_hbridge_on(rt_uint8_t pmos_ch, rt_uint8_t nmos_ch)
{
    if (pmos_ch > HC238_CHANNEL_MAX || nmos_ch > HC238_CHANNEL_MAX)
    {
        return -RT_EINVAL;
    }

    if (pmos_ch == nmos_ch)
    {
        return -RT_EINVAL;
    }

    hc238_bus_enable(RT_FALSE);
    ctrl_h_hc238_select(pmos_ch);
    ctrl_l_hc238_select(nmos_ch);
    hc238_bus_enable(RT_TRUE);
    return RT_EOK;
}

void hc238_hbridge_off(void)
{
    hc238_bus_enable(RT_FALSE);
}

void pair_scan_set_done_cb(pair_scan_done_cb_t cb, void *user_data)
{
    pair_scan_done_cb = cb;
    pair_scan_done_user = user_data;
}

rt_bool_t pair_scan_is_running(void)
{
    return pair_scan_running;
}

const pair_volt_cube_t *pair_scan_get_volt(void)
{
    return &s_volt_mv;
}

static void pair_scan_hw_off(void)
{
    link4051_enable(RT_FALSE);
    hc238_hbridge_off();
    boost_pwr_off();
}

static void pair_volt_clear(void)
{
    int h, l, m;

    for (h = 0; h < PAIR_CH_COUNT; h++)
    {
        for (l = 0; l < PAIR_CH_COUNT; l++)
        {
            for (m = 0; m < PAIR_CH_COUNT; m++)
            {
                s_volt_mv[h][l][m] = PAIR_SCAN_INVALID_MV;
            }
        }
    }
}

/* H/L 已设定：依次采 MUX 0~7，写入 V[h][l][m] */
static void pair_scan_sample_all_mux(int step,
                                    rt_uint8_t pmos_ch,
                                    rt_uint8_t nmos_ch)
{
    rt_uint8_t adc_ch;

    link4051_enable(RT_TRUE);

    for (adc_ch = 0; adc_ch <= HC238_CHANNEL_MAX && !pair_scan_abort; adc_ch++)
    {
        rt_uint32_t pin_mv;
        rt_uint32_t real_mv;

        link4051_select(adc_ch);
        rt_thread_mdelay(PAIR_SCAN_MUX_SETTLE_MS);

        pin_mv = link_adc_read_pin_mv();
        real_mv = link_adc_read_mv();
        if (real_mv > 0xFFFEu)
        {
            real_mv = 0xFFFEu;
        }
        s_volt_mv[pmos_ch][nmos_ch][adc_ch] = (uint16_t)real_mv;

        rt_kprintf("[PAIR_SCAN] %2d/%d  H=Y%u L=Y%u  MUX=Y%u  pin=%umV  V=%u.%03uV\n",
                   step, PAIR_SCAN_TOTAL,
                   pmos_ch, nmos_ch, adc_ch,
                   pin_mv,
                   real_mv / 1000u,
                   real_mv % 1000u);
    }

    link4051_enable(RT_FALSE);
}

static void pair_scan_thread_entry(void *parameter)
{
    rt_uint8_t pmos;
    rt_uint8_t nmos;
    int step = 0;
    rt_bool_t aborted;

    RT_UNUSED(parameter);
    pair_volt_clear();

    boost_pwr_on();
    rt_thread_mdelay(PAIR_SCAN_BOOST_MS);
    rt_kprintf("[PAIR_SCAN] boost on (GPIO%d=HIGH)\n", BOOST_PWR_EN_PIN);
    rt_kprintf("[PAIR_SCAN] start: hl=%dms mux=%dms total=%d\n",
               PAIR_SCAN_HL_SETTLE_MS, PAIR_SCAN_MUX_SETTLE_MS, PAIR_SCAN_TOTAL);

    for (pmos = 0; pmos <= HC238_CHANNEL_MAX && !pair_scan_abort; pmos++)
    {
        for (nmos = 0; nmos <= HC238_CHANNEL_MAX && !pair_scan_abort; nmos++)
        {
            if (nmos == pmos)
            {
                continue;
            }

            step++;
            if (hc238_hbridge_on(pmos, nmos) != RT_EOK)
            {
                rt_kprintf("[PAIR_SCAN] ERROR: H%d L%d rejected\n", pmos, nmos);
                continue;
            }

            rt_thread_mdelay(PAIR_SCAN_HL_SETTLE_MS);
            pair_scan_sample_all_mux(step, pmos, nmos);
            rt_thread_mdelay(PAIR_SCAN_HOLD_MS);
        }
    }

    aborted = pair_scan_abort;
    pair_scan_hw_off();
    pair_scan_running = RT_FALSE;
    pair_scan_tid = RT_NULL;

    if (aborted)
    {
        rt_kprintf("[PAIR_SCAN] aborted after %d/%d H-L steps\n", step, PAIR_SCAN_TOTAL);
    }
    else
    {
        rt_kprintf("[PAIR_SCAN] done, %d/%d H-L steps\n", step, PAIR_SCAN_TOTAL);
    }

    if (pair_scan_done_cb)
    {
        pair_scan_done_cb(aborted, pair_scan_done_user);
    }
}

rt_err_t pair_scan_start(void)
{
    if (pair_scan_running)
    {
        return -RT_EBUSY;
    }

    pair_scan_abort = RT_FALSE;
    pair_scan_running = RT_TRUE;
    pair_scan_tid = rt_thread_create("pair_scan",
                                     pair_scan_thread_entry,
                                     RT_NULL,
                                     PAIR_SCAN_STACK,
                                     PAIR_SCAN_PRIO,
                                     10);
    if (pair_scan_tid == RT_NULL)
    {
        pair_scan_running = RT_FALSE;
        return -RT_ENOMEM;
    }

    rt_thread_startup(pair_scan_tid);
    return RT_EOK;
}

void pair_scan_stop(void)
{
    if (!pair_scan_running)
    {
        pair_scan_hw_off();
        return;
    }

    pair_scan_abort = RT_TRUE;
}

#ifdef RT_USING_FINSH
#include <finsh.h>
#include <stdlib.h>

static rt_err_t _hc238_parse_channel(const char *arg, rt_uint8_t *channel)
{
    int v = atoi(arg);

    if (v < HC238_CHANNEL_MIN || v > HC238_CHANNEL_MAX)
    {
        return -RT_EINVAL;
    }

    *channel = (rt_uint8_t)v;
    return RT_EOK;
}

static void _print_hc238(const char *name, rt_uint8_t a2, rt_uint8_t a1, rt_uint8_t a0, rt_uint8_t channel)
{
    rt_kprintf("  %s: ch=%u (Y%u high when EN), A2/A1/A0=GP%d/GP%d/GP%d\n",
               name, channel, channel, a2, a1, a0);
}

static int cmd_ctrl_h(int argc, char **argv)
{
    rt_uint8_t ch;

    if (argc < 2)
    {
        rt_kprintf("Usage: ctrl_h <0-7>\n");
        return -RT_EINVAL;
    }

    if (_hc238_parse_channel(argv[1], &ch) != RT_EOK)
    {
        rt_kprintf("Invalid channel, use 0-7\n");
        return -RT_EINVAL;
    }

    ctrl_h_hc238_select(ch);
    hc238_bus_enable(RT_TRUE);
    rt_kprintf("CTRL_H PMOS channel %u selected (Y%u high)\n", ch, ch);
    return 0;
}

static int cmd_ctrl_l(int argc, char **argv)
{
    rt_uint8_t ch;

    if (argc < 2)
    {
        rt_kprintf("Usage: ctrl_l <0-7>\n");
        return -RT_EINVAL;
    }

    if (_hc238_parse_channel(argv[1], &ch) != RT_EOK)
    {
        rt_kprintf("Invalid channel, use 0-7\n");
        return -RT_EINVAL;
    }

    ctrl_l_hc238_select(ch);
    hc238_bus_enable(RT_TRUE);
    rt_kprintf("CTRL_L NMOS channel %u selected (Y%u high)\n", ch, ch);
    return 0;
}

static int cmd_hc238_off(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    pair_scan_stop();
    pair_scan_hw_off();
    rt_kprintf("HC238+4051 off, boost off (GPIO%d)\n", BOOST_PWR_EN_PIN);
    return 0;
}

static int cmd_hc238_info(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("74HC238 status (EN=GPIO%d/G2, G0/G1=LOW, %s):\n",
               HC238_EN_PIN, hc238_bus_is_enabled() ? "enabled" : "disabled");
    _print_hc238("CTRL_H PMOS", CTRL_H_A2_PIN, CTRL_H_A1_PIN, CTRL_H_A0_PIN,
                   ctrl_h_hc238_get_channel());
    _print_hc238("CTRL_L NMOS", CTRL_L_A2_PIN, CTRL_L_A1_PIN, CTRL_L_A0_PIN,
                   ctrl_l_hc238_get_channel());
    rt_kprintf("  pair_scan: %s\n", pair_scan_running ? "running" : "idle");
    rt_kprintf("  boost_pwr GPIO%d: %s\n",
               BOOST_PWR_EN_PIN, boost_pwr_is_enabled() ? "on" : "off");
    rt_kprintf("  link4051 GPIO%d EN: %s, ch=%u\n",
               LINK4051_EN_PIN,
               link4051_is_enabled() ? "on" : "off",
               link4051_get_channel());
    return 0;
}

static int cmd_pair_scan(int argc, char **argv)
{
    rt_err_t err;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    err = pair_scan_start();
    if (err == -RT_EBUSY)
    {
        rt_kprintf("pair_scan already running, use pair_scan_stop\n");
        return -RT_EBUSY;
    }
    if (err != RT_EOK)
    {
        rt_kprintf("create pair_scan thread failed\n");
        return (int)err;
    }
    return 0;
}

static int cmd_pair_scan_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!pair_scan_is_running())
    {
        pair_scan_hw_off();
        rt_kprintf("pair_scan not running\n");
        return 0;
    }

    pair_scan_stop();
    rt_kprintf("pair_scan stop requested...\n");
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_ctrl_h, ctrl_h, CTRL_H PMOS hc238 select 0-7);
MSH_CMD_EXPORT_ALIAS(cmd_ctrl_l, ctrl_l, CTRL_L NMOS hc238 select 0-7);
MSH_CMD_EXPORT_ALIAS(cmd_hc238_off, hc238_off, disable both hc238);
MSH_CMD_EXPORT_ALIAS(cmd_hc238_info, hc238_info, show hc238 status);
MSH_CMD_EXPORT_ALIAS(cmd_pair_scan, pair_scan, H-bridge pair scan with ADC);
MSH_CMD_EXPORT_ALIAS(cmd_pair_scan_stop, pair_scan_stop, stop pair_scan);
#endif /* RT_USING_FINSH */

static int hc238_auto_init(void)
{
    hc238_bus_init();
    ctrl_h_hc238_init();
    ctrl_l_hc238_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(hc238_auto_init);
