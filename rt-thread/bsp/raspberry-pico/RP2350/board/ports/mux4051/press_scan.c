/*
 * 压接扫描与 FinSH 调试命令
 *
 * 流程：
 * 1. enable TX 4051
 * 2. SC_PWM 输出 460kHz 方波
 * 3. enable RX 4051
 * 4. 对每个 TX 通道 0..7：选通 TX，再依次选通 RX（跳过与 TX 相同通道），
 *    延时 50ms 后采样 GPIO45/ADC5
 */
#include "press_scan.h"
#include "tx_mux4051.h"
#include "rx_mux4051.h"
#include "rx_adc.h"
#include "sc_link_pwm.h"

#include <string.h>

int press_scan_run(rt_uint32_t mv[PRESS_SCAN_CH_NUM][PRESS_SCAN_CH_NUM])
{
    rt_uint8_t tx;
    rt_uint8_t rx;
    int count = 0;

    if (mv == RT_NULL)
    {
        return -RT_EINVAL;
    }

    rt_memset(mv, 0, sizeof(rt_uint32_t) * PRESS_SCAN_CH_NUM * PRESS_SCAN_CH_NUM);

    rx_adc_init();
    tx_mux4051_init();
    rx_mux4051_init();

    /* 1. 打开发送 4051 */
    tx_mux4051_enable(RT_TRUE);

    /* 2. send_pwm = SC_PWM @ 460kHz */
    if (sc_pwm_start(CARRIER_PWM_FREQ_460KHZ) != RT_EOK)
    {
        tx_mux4051_enable(RT_FALSE);
        return -RT_ERROR;
    }

    /* 3. 打开接收 4051 */
    rx_mux4051_enable(RT_TRUE);

    for (tx = 0; tx < PRESS_SCAN_CH_NUM; tx++)
    {
        tx_mux4051_select(tx);

        for (rx = 0; rx < PRESS_SCAN_CH_NUM; rx++)
        {
            /* TX/RX 禁止同通道，也不采样同通道 */
            if (rx == tx)
            {
                continue;
            }

            rx_mux4051_select(rx);
            rt_thread_mdelay(PRESS_SCAN_SETTLE_MS);
            mv[tx][rx] = rx_adc_read_pin_mv();
            count++;
        }
    }

    /* 收尾：关 PWM、关 mux */
    sc_pwm_stop();
    rx_mux4051_enable(RT_FALSE);
    tx_mux4051_enable(RT_FALSE);

    return count;
}

void press_scan_rx_avg(const rt_uint32_t mv[PRESS_SCAN_CH_NUM][PRESS_SCAN_CH_NUM],
                       rt_uint32_t avg_mv[PRESS_SCAN_CH_NUM])
{
    rt_uint8_t tx;
    rt_uint8_t rx;
    rt_uint32_t sum;

    if (mv == RT_NULL || avg_mv == RT_NULL)
    {
        return;
    }

    for (rx = 0; rx < PRESS_SCAN_CH_NUM; rx++)
    {
        sum = 0;
        for (tx = 0; tx < PRESS_SCAN_CH_NUM; tx++)
        {
            if (tx == rx)
            {
                continue;
            }
            sum += mv[tx][rx];
        }
        avg_mv[rx] = sum / PRESS_SCAN_RX_AVG_N;
    }
}

#ifdef RT_USING_FINSH
#include <finsh.h>

static void press_scan_print_matrix(const rt_uint32_t mv[PRESS_SCAN_CH_NUM][PRESS_SCAN_CH_NUM])
{
    rt_uint8_t tx;
    rt_uint8_t rx;

    rt_kprintf("\nPress scan ADC pin mV (GPIO%d/ADC%d), settle=%dms\n",
               RX_ADC_GPIO, RX_ADC_CHANNEL, PRESS_SCAN_SETTLE_MS);
    rt_kprintf("TX\\RX");
    for (rx = 0; rx < PRESS_SCAN_CH_NUM; rx++)
    {
        rt_kprintf("%6u", rx);
    }
    rt_kprintf("\n");

    for (tx = 0; tx < PRESS_SCAN_CH_NUM; tx++)
    {
        rt_kprintf("  %u  ", tx);
        for (rx = 0; rx < PRESS_SCAN_CH_NUM; rx++)
        {
            if (tx == rx)
            {
                rt_kprintf("   ---");
            }
            else
            {
                rt_kprintf("%6u", mv[tx][rx]);
            }
        }
        rt_kprintf("\n");
    }
}

static int cmd_press_scan(int argc, char **argv)
{
    rt_uint32_t mv[PRESS_SCAN_CH_NUM][PRESS_SCAN_CH_NUM];
    rt_uint32_t avg_mv[PRESS_SCAN_CH_NUM];
    rt_uint8_t tx;
    rt_uint8_t rx;
    int n;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    rt_kprintf("press_scan: enable TX mux, SC_PWM 460kHz, scan RX mux...\n");
    n = press_scan_run(mv);
    if (n < 0)
    {
        rt_kprintf("press_scan failed: %d\n", n);
        return n;
    }

    /* 逐点打印，便于串口实时查看 */
    for (tx = 0; tx < PRESS_SCAN_CH_NUM; tx++)
    {
        for (rx = 0; rx < PRESS_SCAN_CH_NUM; rx++)
        {
            if (tx == rx)
            {
                continue;
            }
            rt_kprintf("TX%u->RX%u: %u mV\n", tx, rx, mv[tx][rx]);
        }
    }

    press_scan_print_matrix(mv);

    /* 每个 RX：对其余 7 个 TX 采样取均值 */
    press_scan_rx_avg(mv, avg_mv);
    rt_kprintf("\nRX avg over %d TX samples (mV):\n", PRESS_SCAN_RX_AVG_N);
    for (rx = 0; rx < PRESS_SCAN_CH_NUM; rx++)
    {
        rt_kprintf("RX%u avg: %u mV\n", rx, avg_mv[rx]);
    }

    rt_kprintf("done, samples=%d\n", n);
    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_press_scan, press_scan, press scan TX/RX 4051 print ADC mV);
#endif /* RT_USING_FINSH */
