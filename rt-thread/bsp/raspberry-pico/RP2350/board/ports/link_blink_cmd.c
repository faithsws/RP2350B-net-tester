/*
 * 找端口 / 端口闪烁：反复关闭/打开 CH390 PHY，使交换机口 Link 灯闪烁
 *
 * 用法:
 *   link_blink [周期ms] [次数]   默认 2000ms、10 次；次数=0 一直闪到 stop
 *   link_blink_stop              停止并恢复 PHY 上电
 */
#include <rtthread.h>

#if defined(BSP_USING_CH390) && defined(RT_USING_FINSH)

#include <stdlib.h>
#include <ch390.h>
#include <finsh.h>

#define LINK_BLINK_DEFAULT_PERIOD_MS  2000
#define LINK_BLINK_DEFAULT_COUNT      10
#define LINK_BLINK_STACK              1024
#define LINK_BLINK_PRIO               (RT_THREAD_PRIORITY_MAX / 2 + 4)

static volatile rt_bool_t link_blink_running = RT_FALSE;
static volatile rt_bool_t link_blink_abort = RT_FALSE;
static rt_thread_t link_blink_tid = RT_NULL;

struct link_blink_param
{
    int period_ms;
    int count;
};

static void link_blink_thread_entry(void *parameter)
{
    struct link_blink_param *p = (struct link_blink_param *)parameter;
    int period_ms;
    int count;
    int down_ms;
    int up_ms;
    int i;

    period_ms = (p && p->period_ms > 0) ? p->period_ms : LINK_BLINK_DEFAULT_PERIOD_MS;
    count = (p) ? p->count : LINK_BLINK_DEFAULT_COUNT;
    if (p)
    {
        rt_free(p);
    }

    /* 半周期关链、半周期开链；开链至少 800ms 便于协商亮灯 */
    down_ms = period_ms / 2;
    up_ms = period_ms - down_ms;
    if (down_ms < 200)
    {
        down_ms = 200;
    }
    if (up_ms < 800)
    {
        up_ms = 800;
    }

    if (count > 0)
    {
        rt_kprintf("[LINK_BLINK] start: down=%dms up=%dms count=%d\n",
                   down_ms, up_ms, count);
        rt_kprintf("[LINK_BLINK] watch switch port Link LED\n");
    }
    else
    {
        rt_kprintf("[LINK_BLINK] start: down=%dms up=%dms until stop\n",
                   down_ms, up_ms);
    }

    i = 0;
    while (!link_blink_abort && (count <= 0 || i < count))
    {
        i++;
        rt_kprintf("[LINK_BLINK] #%d PHY off (link down)\n", i);
        if (ch390_phy_power_set(RT_FALSE) != RT_EOK)
        {
            rt_kprintf("[LINK_BLINK] PHY off failed\n");
            break;
        }
        rt_thread_mdelay(down_ms);

        if (link_blink_abort)
        {
            break;
        }

        rt_kprintf("[LINK_BLINK] #%d PHY on  (negotiate)\n", i);
        if (ch390_phy_power_set(RT_TRUE) != RT_EOK)
        {
            rt_kprintf("[LINK_BLINK] PHY on failed\n");
            break;
        }
        rt_thread_mdelay(up_ms);
    }

    /* 结束时确保 PHY 上电 */
    ch390_phy_power_set(RT_TRUE);
    link_blink_running = RT_FALSE;
    link_blink_tid = RT_NULL;
    rt_kprintf("[LINK_BLINK] stopped, cycles=%d, link=%s\n",
               i, ch390_link_is_up() ? "up" : "down(wait)");
}

static int cmd_link_blink(int argc, char **argv)
{
    int period_ms = LINK_BLINK_DEFAULT_PERIOD_MS;
    int count = LINK_BLINK_DEFAULT_COUNT;
    struct link_blink_param *param;

    if (link_blink_running)
    {
        rt_kprintf("[LINK_BLINK] already running, use link_blink_stop\n");
        return -1;
    }

    if (argc >= 2)
    {
        period_ms = atoi(argv[1]);
        if (period_ms < 400)
        {
            period_ms = 400;
        }
    }
    if (argc >= 3)
    {
        count = atoi(argv[2]);
    }

    if (argc == 1)
    {
        rt_kprintf("Usage: link_blink [period_ms] [count]\n");
        rt_kprintf("  period_ms  one blink cycle (down+up), default %d\n",
                   LINK_BLINK_DEFAULT_PERIOD_MS);
        rt_kprintf("  count      times, 0=until stop, default %d\n",
                   LINK_BLINK_DEFAULT_COUNT);
    }

    param = (struct link_blink_param *)rt_malloc(sizeof(*param));
    if (param == RT_NULL)
    {
        rt_kprintf("[LINK_BLINK] malloc failed\n");
        return -1;
    }
    param->period_ms = period_ms;
    param->count = count;

    link_blink_abort = RT_FALSE;
    link_blink_running = RT_TRUE;
    link_blink_tid = rt_thread_create("lnk_blk",
                                      link_blink_thread_entry,
                                      param,
                                      LINK_BLINK_STACK,
                                      LINK_BLINK_PRIO,
                                      10);
    if (link_blink_tid == RT_NULL)
    {
        rt_free(param);
        link_blink_running = RT_FALSE;
        rt_kprintf("[LINK_BLINK] create thread failed\n");
        return -1;
    }

    rt_thread_startup(link_blink_tid);
    return 0;
}

static int cmd_link_blink_stop(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (!link_blink_running)
    {
        ch390_phy_power_set(RT_TRUE);
        rt_kprintf("[LINK_BLINK] not running, PHY forced on\n");
        return 0;
    }

    link_blink_abort = RT_TRUE;
    rt_kprintf("[LINK_BLINK] stop requested...\n");
    return 0;
}

MSH_CMD_EXPORT_ALIAS(cmd_link_blink, link_blink, blink switch port LED by link up/down);
MSH_CMD_EXPORT_ALIAS(cmd_link_blink_stop, link_blink_stop, stop link blink and restore PHY);

#endif /* BSP_USING_CH390 && RT_USING_FINSH */
