/*
 * SC_PWM (GPIO17) / LINK_PWM (GPIO19) 载波 PWM 驱动
 */
#include "sc_link_pwm.h"
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pwm.h>

typedef struct
{
    rt_uint8_t pin;
    rt_uint8_t slice;
    rt_uint8_t channel;
    rt_bool_t running;
    carrier_pwm_freq_t freq_khz;
} carrier_pwm_ctx_t;

static carrier_pwm_ctx_t sc_pwm_ctx =
{
    .pin = SC_PWM_PIN,
    .running = RT_FALSE,
    .freq_khz = CARRIER_PWM_FREQ_455KHZ,
};

static carrier_pwm_ctx_t link_pwm_ctx =
{
    .pin = LINK_PWM_PIN,
    .running = RT_FALSE,
    .freq_khz = CARRIER_PWM_FREQ_455KHZ,
};

static rt_bool_t carrier_pwm_freq_valid(carrier_pwm_freq_t freq)
{
    return (freq == CARRIER_PWM_FREQ_455KHZ || freq == CARRIER_PWM_FREQ_460KHZ);
}

static rt_uint32_t carrier_pwm_calc_top(rt_uint32_t sys_clk_hz, rt_uint32_t freq_hz)
{
    rt_uint32_t period = (sys_clk_hz + freq_hz / 2U) / freq_hz;

    if (period < 2U)
    {
        period = 2U;
    }

    return period - 1U;
}

static rt_err_t carrier_pwm_apply(carrier_pwm_ctx_t *ctx, carrier_pwm_freq_t freq)
{
    rt_uint32_t freq_hz;
    rt_uint32_t sys_clk_hz;
    rt_uint32_t top;
    rt_uint32_t level;

    if (!carrier_pwm_freq_valid(freq))
    {
        return -RT_EINVAL;
    }

    freq_hz = (rt_uint32_t)freq * 1000U;
    sys_clk_hz = clock_get_hz(clk_sys);
    top = carrier_pwm_calc_top(sys_clk_hz, freq_hz);
    level = (top + 1U) / 2U;

    ctx->slice = (rt_uint8_t)pwm_gpio_to_slice_num(ctx->pin);
    ctx->channel = (rt_uint8_t)pwm_gpio_to_channel(ctx->pin);

    gpio_set_function(ctx->pin, GPIO_FUNC_PWM);

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, 1.0f);
    pwm_config_set_wrap(&cfg, top);
    pwm_init(ctx->slice, &cfg, true);
    pwm_set_chan_level(ctx->slice, ctx->channel, level);
    pwm_set_enabled(ctx->slice, true);

    ctx->running = RT_TRUE;
    ctx->freq_khz = freq;
    return RT_EOK;
}

static rt_err_t carrier_pwm_stop_ctx(carrier_pwm_ctx_t *ctx)
{
    if (ctx->running)
    {
        pwm_set_enabled(ctx->slice, false);
        gpio_set_function(ctx->pin, GPIO_FUNC_SIO);
        gpio_set_dir(ctx->pin, GPIO_OUT);
        gpio_put(ctx->pin, 0);
        ctx->running = RT_FALSE;
    }

    return RT_EOK;
}

void sc_link_pwm_init(void)
{
    sc_pwm_ctx.slice = (rt_uint8_t)pwm_gpio_to_slice_num(SC_PWM_PIN);
    sc_pwm_ctx.channel = (rt_uint8_t)pwm_gpio_to_channel(SC_PWM_PIN);
    link_pwm_ctx.slice = (rt_uint8_t)pwm_gpio_to_slice_num(LINK_PWM_PIN);
    link_pwm_ctx.channel = (rt_uint8_t)pwm_gpio_to_channel(LINK_PWM_PIN);
}

rt_err_t sc_pwm_start(carrier_pwm_freq_t freq)
{
    return carrier_pwm_apply(&sc_pwm_ctx, freq);
}

rt_err_t sc_pwm_stop(void)
{
    return carrier_pwm_stop_ctx(&sc_pwm_ctx);
}

rt_bool_t sc_pwm_is_running(void)
{
    return sc_pwm_ctx.running;
}

carrier_pwm_freq_t sc_pwm_get_freq(void)
{
    return sc_pwm_ctx.freq_khz;
}

rt_err_t link_pwm_start(carrier_pwm_freq_t freq)
{
    return carrier_pwm_apply(&link_pwm_ctx, freq);
}

rt_err_t link_pwm_stop(void)
{
    return carrier_pwm_stop_ctx(&link_pwm_ctx);
}

rt_bool_t link_pwm_is_running(void)
{
    return link_pwm_ctx.running;
}

carrier_pwm_freq_t link_pwm_get_freq(void)
{
    return link_pwm_ctx.freq_khz;
}

static int sc_link_pwm_auto_init(void)
{
    sc_link_pwm_init();
    return RT_EOK;
}
INIT_DEVICE_EXPORT(sc_link_pwm_auto_init);

#ifdef RT_USING_FINSH
#include <finsh.h>
#include <stdlib.h>

static carrier_pwm_freq_t _parse_freq_khz(const char *arg)
{
    int v = atoi(arg);

    if (v == 455)
    {
        return CARRIER_PWM_FREQ_455KHZ;
    }
    if (v == 460)
    {
        return CARRIER_PWM_FREQ_460KHZ;
    }
    return (carrier_pwm_freq_t)0;
}

static void _print_pwm_status(const char *name, rt_uint8_t pin, carrier_pwm_ctx_t *ctx)
{
    if (ctx->running)
    {
        rt_kprintf("  %s (GPIO%d): running, %ukHz, slice=%u ch=%c\n",
                   name, pin, (rt_uint32_t)ctx->freq_khz, ctx->slice,
                   ctx->channel ? 'B' : 'A');
    }
    else
    {
        rt_kprintf("  %s (GPIO%d): stopped\n", name, pin);
    }
}

static int cmd_sc_pwm(int argc, char **argv)
{
    carrier_pwm_freq_t freq;

    if (argc < 2)
    {
        rt_kprintf("Usage: sc_pwm <455|460|off>\n");
        return -RT_EINVAL;
    }

    if (!rt_strcmp(argv[1], "off"))
    {
        sc_pwm_stop();
        rt_kprintf("SC_PWM stopped\n");
        return 0;
    }

    freq = _parse_freq_khz(argv[1]);
    if (freq == 0)
    {
        rt_kprintf("Invalid freq, use 455 or 460\n");
        return -RT_EINVAL;
    }

    if (sc_pwm_start(freq) == RT_EOK)
    {
        rt_kprintf("SC_PWM started: %ukHz on GPIO%d\n", (rt_uint32_t)freq, SC_PWM_PIN);
    }
    return 0;
}

static int cmd_link_pwm(int argc, char **argv)
{
    carrier_pwm_freq_t freq;

    if (argc < 2)
    {
        rt_kprintf("Usage: link_pwm <455|460|off>\n");
        return -RT_EINVAL;
    }

    if (!rt_strcmp(argv[1], "off"))
    {
        link_pwm_stop();
        rt_kprintf("LINK_PWM stopped\n");
        return 0;
    }

    freq = _parse_freq_khz(argv[1]);
    if (freq == 0)
    {
        rt_kprintf("Invalid freq, use 455 or 460\n");
        return -RT_EINVAL;
    }

    if (link_pwm_start(freq) == RT_EOK)
    {
        rt_kprintf("LINK_PWM started: %ukHz on GPIO%d\n", (rt_uint32_t)freq, LINK_PWM_PIN);
    }
    return 0;
}

static int cmd_pwm_info(int argc, char **argv)
{
    rt_kprintf("Carrier PWM status (sys_clk=%u Hz):\n", clock_get_hz(clk_sys));
    _print_pwm_status("SC_PWM", SC_PWM_PIN, &sc_pwm_ctx);
    _print_pwm_status("LINK_PWM", LINK_PWM_PIN, &link_pwm_ctx);
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_sc_pwm, sc_pwm, SC_PWM output 455/460kHz on GPIO17);
MSH_CMD_EXPORT_ALIAS(cmd_link_pwm, link_pwm, LINK_PWM output 455/460kHz on GPIO19);
MSH_CMD_EXPORT_ALIAS(cmd_pwm_info, pwm_info, show SC/LINK PWM status);
#endif /* RT_USING_FINSH */
