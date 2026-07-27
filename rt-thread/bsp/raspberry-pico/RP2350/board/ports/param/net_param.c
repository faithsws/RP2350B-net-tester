/*
 * 功能参数：INI 存 FAL param 分区，开机加载，FinSH 可查改
 */
#include "net_param.h"
#include "press_scan.h"

#include <fal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <hardware/flash.h>

#define NET_PARAM_PART_NAME     "param"
#define NET_PARAM_SECTOR_SIZE   4096
#define NET_PARAM_MAGIC_LINE    "# net-tester-param v1"

typedef struct
{
    const char *key;
    rt_size_t offset;
    rt_uint32_t def;
} net_param_item_t;

#define NET_PARAM_ITEM(name, field, def) \
    { name, offsetof(net_param_t, field), (def) }

static const net_param_item_t s_items[] =
{
    NET_PARAM_ITEM("press.judge_thr_mv",   press_judge_thr_mv,   NET_PARAM_PRESS_JUDGE_THR_MV_DEFAULT),

    NET_PARAM_ITEM("pair.tol_mv",          pair_tol_mv,          NET_PARAM_PAIR_TOL_MV_DEFAULT),
    NET_PARAM_ITEM("pair.conn_remote_mv",  pair_conn_remote_mv,  NET_PARAM_PAIR_CONN_REMOTE_MV_DEFAULT),
    NET_PARAM_ITEM("pair.conn_h_mv",       pair_conn_h_mv,       NET_PARAM_PAIR_CONN_H_MV_DEFAULT),
    NET_PARAM_ITEM("pair.conn_l_mv",       pair_conn_l_mv,       NET_PARAM_PAIR_CONN_L_MV_DEFAULT),
    NET_PARAM_ITEM("pair.short_mh_hi_mv",  pair_short_mh_hi_mv,  NET_PARAM_PAIR_SHORT_MH_HI_MV_DEFAULT),
    NET_PARAM_ITEM("pair.short_mh_mid_mv", pair_short_mh_mid_mv, NET_PARAM_PAIR_SHORT_MH_MID_MV_DEFAULT),
    NET_PARAM_ITEM("pair.short_lh_lo_mv",  pair_short_lh_lo_mv,  NET_PARAM_PAIR_SHORT_LH_LO_MV_DEFAULT),
    NET_PARAM_ITEM("pair.short_lh_mid_mv", pair_short_lh_mid_mv, NET_PARAM_PAIR_SHORT_LH_MID_MV_DEFAULT),

    NET_PARAM_ITEM("seq.min_mv",           seq_min_mv,           NET_PARAM_SEQ_MIN_MV_DEFAULT),

    NET_PARAM_ITEM("bat.lvl4_mv",          bat_lvl4_mv,          NET_PARAM_BAT_LVL4_MV_DEFAULT),
    NET_PARAM_ITEM("bat.lvl3_mv",          bat_lvl3_mv,          NET_PARAM_BAT_LVL3_MV_DEFAULT),
    NET_PARAM_ITEM("bat.lvl2_mv",          bat_lvl2_mv,          NET_PARAM_BAT_LVL2_MV_DEFAULT),
    NET_PARAM_ITEM("bat.low_mv",           bat_low_mv,           NET_PARAM_BAT_LOW_MV_DEFAULT),
};

#define NET_PARAM_ITEM_CNT  (sizeof(s_items) / sizeof(s_items[0]))

static net_param_t g_param = {
    .press_judge_thr_mv   = NET_PARAM_PRESS_JUDGE_THR_MV_DEFAULT,
    .pair_tol_mv          = NET_PARAM_PAIR_TOL_MV_DEFAULT,
    .pair_conn_remote_mv  = NET_PARAM_PAIR_CONN_REMOTE_MV_DEFAULT,
    .pair_conn_h_mv       = NET_PARAM_PAIR_CONN_H_MV_DEFAULT,
    .pair_conn_l_mv       = NET_PARAM_PAIR_CONN_L_MV_DEFAULT,
    .pair_short_mh_hi_mv  = NET_PARAM_PAIR_SHORT_MH_HI_MV_DEFAULT,
    .pair_short_mh_mid_mv = NET_PARAM_PAIR_SHORT_MH_MID_MV_DEFAULT,
    .pair_short_lh_lo_mv  = NET_PARAM_PAIR_SHORT_LH_LO_MV_DEFAULT,
    .pair_short_lh_mid_mv = NET_PARAM_PAIR_SHORT_LH_MID_MV_DEFAULT,
    .seq_min_mv           = NET_PARAM_SEQ_MIN_MV_DEFAULT,
    .bat_lvl4_mv          = NET_PARAM_BAT_LVL4_MV_DEFAULT,
    .bat_lvl3_mv          = NET_PARAM_BAT_LVL3_MV_DEFAULT,
    .bat_lvl2_mv          = NET_PARAM_BAT_LVL2_MV_DEFAULT,
    .bat_low_mv           = NET_PARAM_BAT_LOW_MV_DEFAULT,
};

static rt_uint32_t *net_param_field(rt_size_t offset)
{
    return (rt_uint32_t *)((rt_uint8_t *)&g_param + offset);
}

static const net_param_item_t *net_param_find_item(const char *key)
{
    rt_size_t i;

    for (i = 0; i < NET_PARAM_ITEM_CNT; i++)
    {
        if (strcmp(s_items[i].key, key) == 0)
        {
            return &s_items[i];
        }
    }
    return RT_NULL;
}

static void net_param_apply_runtime(void)
{
    press_judge_thr_set(g_param.press_judge_thr_mv);
}

void net_param_set_defaults(void)
{
    rt_size_t i;

    for (i = 0; i < NET_PARAM_ITEM_CNT; i++)
    {
        *net_param_field(s_items[i].offset) = s_items[i].def;
    }
}

const net_param_t *net_param_get(void)
{
    return &g_param;
}

rt_uint32_t net_param_press_judge_thr_mv(void)
{
    return g_param.press_judge_thr_mv;
}

void net_param_set_press_judge_thr_mv(rt_uint32_t mv)
{
    if (mv == 0)
    {
        mv = NET_PARAM_PRESS_JUDGE_THR_MV_DEFAULT;
    }
    g_param.press_judge_thr_mv = mv;
    net_param_apply_runtime();
}

static void net_param_parse_line(char *line)
{
    char *eq;
    char *key;
    char *val;
    const net_param_item_t *item;

    while (*line == ' ' || *line == '\t')
    {
        line++;
    }
    if (*line == '\0' || *line == '#' || *line == ';')
    {
        return;
    }

    eq = strchr(line, '=');
    if (eq == RT_NULL)
    {
        return;
    }
    *eq = '\0';
    key = line;
    val = eq + 1;

    {
        char *p = key + strlen(key);
        while (p > key && (p[-1] == ' ' || p[-1] == '\t'))
        {
            *--p = '\0';
        }
    }
    while (*val == ' ' || *val == '\t')
    {
        val++;
    }

    item = net_param_find_item(key);
    if (item)
    {
        *net_param_field(item->offset) = (rt_uint32_t)strtoul(val, RT_NULL, 0);
    }
}

static rt_err_t net_param_parse_ini(char *text)
{
    char *p = text;

    net_param_set_defaults();

    while (p && *p)
    {
        char *line = p;
        char *nl = p;

        while (*nl && *nl != '\n' && *nl != '\r')
        {
            nl++;
        }
        if (*nl)
        {
            *nl++ = '\0';
            if (*nl == '\n' || *nl == '\r')
            {
                nl++;
            }
            p = nl;
        }
        else
        {
            p = RT_NULL;
        }
        net_param_parse_line(line);
    }
    return RT_EOK;
}

static int net_param_build_ini(char *buf, int bufsz)
{
    int pos = 0;
    rt_size_t i;
    int n;

    n = rt_snprintf(buf + pos, bufsz - pos, "%s\n", NET_PARAM_MAGIC_LINE);
    if (n < 0)
    {
        return -1;
    }
    pos += n;

    for (i = 0; i < NET_PARAM_ITEM_CNT; i++)
    {
        n = rt_snprintf(buf + pos, bufsz - pos, "%s=%u\n",
                        s_items[i].key,
                        (unsigned)*net_param_field(s_items[i].offset));
        if (n < 0 || pos + n >= bufsz)
        {
            return -1;
        }
        pos += n;
    }
    return pos;
}

rt_err_t net_param_load(void)
{
    const struct fal_partition *part;
    char *buf;
    int n;

    net_param_set_defaults();

    part = fal_partition_find(NET_PARAM_PART_NAME);
    if (part == RT_NULL)
    {
        rt_kprintf("[param] partition \"%s\" not found, use defaults\n",
                   NET_PARAM_PART_NAME);
        net_param_apply_runtime();
        return -RT_ERROR;
    }

    buf = rt_malloc(NET_PARAM_SECTOR_SIZE + 1);
    if (buf == RT_NULL)
    {
        net_param_apply_runtime();
        return -RT_ENOMEM;
    }

    n = fal_partition_read(part, 0, (rt_uint8_t *)buf, NET_PARAM_SECTOR_SIZE);
    if (n < 0)
    {
        rt_free(buf);
        rt_kprintf("[param] flash read failed, use defaults\n");
        net_param_apply_runtime();
        return -RT_ERROR;
    }

    if ((rt_uint8_t)buf[0] == 0xFF)
    {
        rt_free(buf);
        rt_kprintf("[param] empty, use defaults\n");
        net_param_apply_runtime();
        return RT_EOK;
    }

    buf[NET_PARAM_SECTOR_SIZE] = '\0';
    {
        int i;
        for (i = 0; i < NET_PARAM_SECTOR_SIZE; i++)
        {
            if ((rt_uint8_t)buf[i] == 0xFF)
            {
                buf[i] = '\0';
                break;
            }
        }
    }

    if (strstr(buf, "net-tester-param") == RT_NULL)
    {
        rt_kprintf("[param] bad magic, use defaults\n");
        net_param_set_defaults();
    }
    else
    {
        net_param_parse_ini(buf);
        rt_kprintf("[param] loaded %u keys (press=%u pair.tol=%u bat.low=%u)\n",
                   (unsigned)NET_PARAM_ITEM_CNT,
                   (unsigned)g_param.press_judge_thr_mv,
                   (unsigned)g_param.pair_tol_mv,
                   (unsigned)g_param.bat_low_mv);
    }

    rt_free(buf);
    net_param_apply_runtime();
    return RT_EOK;
}

rt_err_t net_param_save(void)
{
    const struct fal_partition *part;
    char *buf;
    int len;
    int ret;

    part = fal_partition_find(NET_PARAM_PART_NAME);
    if (part == RT_NULL)
    {
        return -RT_ERROR;
    }

    buf = rt_malloc(NET_PARAM_SECTOR_SIZE);
    if (buf == RT_NULL)
    {
        return -RT_ENOMEM;
    }
    rt_memset(buf, 0xFF, NET_PARAM_SECTOR_SIZE);

    len = net_param_build_ini(buf, NET_PARAM_SECTOR_SIZE);
    if (len < 0 || len >= NET_PARAM_SECTOR_SIZE)
    {
        rt_free(buf);
        return -RT_ERROR;
    }
    /* 文本后保持 0xFF，不要写入 '\\0'，避免 dump/%s 误截断 */

    ret = fal_partition_erase(part, 0, NET_PARAM_SECTOR_SIZE);
    if (ret < 0)
    {
        rt_free(buf);
        rt_kprintf("[param] erase failed\n");
        return -RT_ERROR;
    }

    /* 按页对齐写入（至少覆盖全部文本） */
    {
        int wr = (len + FLASH_PAGE_SIZE - 1) & ~(FLASH_PAGE_SIZE - 1);
        if (wr < FLASH_PAGE_SIZE)
        {
            wr = FLASH_PAGE_SIZE;
        }
        if (wr > NET_PARAM_SECTOR_SIZE)
        {
            wr = NET_PARAM_SECTOR_SIZE;
        }
        ret = fal_partition_write(part, 0, (rt_uint8_t *)buf, wr);
    }
    if (ret < 0)
    {
        rt_free(buf);
        rt_kprintf("[param] write failed\n");
        return -RT_ERROR;
    }

    /* 回读校验 */
    {
        char *chk = rt_malloc(NET_PARAM_SECTOR_SIZE + 1);
        if (chk)
        {
            if (fal_partition_read(part, 0, (rt_uint8_t *)chk, NET_PARAM_SECTOR_SIZE) >= 0)
            {
                chk[NET_PARAM_SECTOR_SIZE] = '\0';
                if (rt_memcmp(chk, buf, (rt_size_t)len) != 0)
                {
                    rt_kprintf("[param] verify FAILED (len=%d)\n", len);
                    rt_free(chk);
                    rt_free(buf);
                    return -RT_ERROR;
                }
            }
            rt_free(chk);
        }
    }

    rt_free(buf);
    rt_kprintf("[param] saved (%d bytes)\n", len);
    return RT_EOK;
}

static int net_param_boot_init(void)
{
    net_param_load();
    return 0;
}
INIT_APP_EXPORT(net_param_boot_init);

#ifdef RT_USING_FINSH
#include <finsh.h>

static void param_show(void)
{
    rt_size_t i;

    for (i = 0; i < NET_PARAM_ITEM_CNT; i++)
    {
        rt_kprintf("%-22s = %u  (default %u)\n",
                   s_items[i].key,
                   (unsigned)*net_param_field(s_items[i].offset),
                   (unsigned)s_items[i].def);
    }
}

static void param_usage(void)
{
    rt_size_t i;

    rt_kprintf("Usage:\n");
    rt_kprintf("  param / param show\n");
    rt_kprintf("  param get <key>\n");
    rt_kprintf("  param set <key> <val>\n");
    rt_kprintf("  param save | load | reset | dump\n");
    rt_kprintf("Keys:\n");
    for (i = 0; i < NET_PARAM_ITEM_CNT; i++)
    {
        rt_kprintf("  %s\n", s_items[i].key);
    }
}

static int param(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "show") == 0)
    {
        param_show();
        return 0;
    }
    if (strcmp(argv[1], "help") == 0)
    {
        param_usage();
        return 0;
    }
    if (strcmp(argv[1], "load") == 0)
    {
        return (net_param_load() == RT_EOK) ? 0 : -1;
    }
    if (strcmp(argv[1], "save") == 0)
    {
        return (net_param_save() == RT_EOK) ? 0 : -1;
    }
    if (strcmp(argv[1], "reset") == 0)
    {
        net_param_set_defaults();
        net_param_apply_runtime();
        param_show();
        rt_kprintf("(RAM only; run \"param save\" to persist)\n");
        return 0;
    }
    if (strcmp(argv[1], "dump") == 0)
    {
        const struct fal_partition *part = fal_partition_find(NET_PARAM_PART_NAME);
        char *buf;
        int i;

        if (part == RT_NULL)
        {
            return -1;
        }
        buf = rt_malloc(NET_PARAM_SECTOR_SIZE + 1);
        if (buf == RT_NULL)
        {
            return -1;
        }
        if (fal_partition_read(part, 0, (rt_uint8_t *)buf, NET_PARAM_SECTOR_SIZE) < 0)
        {
            rt_free(buf);
            return -1;
        }
        for (i = 0; i < NET_PARAM_SECTOR_SIZE; i++)
        {
            if ((rt_uint8_t)buf[i] == 0xFF)
            {
                buf[i] = '\0';
                break;
            }
        }
        buf[NET_PARAM_SECTOR_SIZE] = '\0';
        /* 按字节打印到首个 0xFF，避免中间 0x00 截断 */
        rt_kprintf("---- flash INI ----\n");
        for (i = 0; i < NET_PARAM_SECTOR_SIZE; i++)
        {
            rt_uint8_t c = (rt_uint8_t)buf[i];
            if (c == 0xFF)
            {
                break;
            }
            if (c == '\0')
            {
                continue;
            }
            rt_kprintf("%c", (char)c);
        }
        if (i == 0)
        {
            rt_kprintf("(empty)");
        }
        rt_kprintf("\n---- end ----\n");
        rt_free(buf);
        return 0;
    }
    if (strcmp(argv[1], "get") == 0 && argc >= 3)
    {
        const net_param_item_t *item = net_param_find_item(argv[2]);
        if (!item)
        {
            rt_kprintf("unknown key: %s\n", argv[2]);
            return -1;
        }
        rt_kprintf("%u\n", (unsigned)*net_param_field(item->offset));
        return 0;
    }
    if (strcmp(argv[1], "set") == 0 && argc >= 4)
    {
        const net_param_item_t *item = net_param_find_item(argv[2]);
        rt_uint32_t mv;

        if (!item)
        {
            rt_kprintf("unknown key: %s\n", argv[2]);
            return -1;
        }
        mv = (rt_uint32_t)strtoul(argv[3], RT_NULL, 0);
        *net_param_field(item->offset) = mv;
        if (item->offset == offsetof(net_param_t, press_judge_thr_mv))
        {
            net_param_apply_runtime();
        }
        rt_kprintf("%s = %u (RAM; run \"param save\")\n", item->key, (unsigned)mv);
        return 0;
    }

    param_usage();
    return -1;
}
MSH_CMD_EXPORT(param, view/set/save net-tester params);
#endif /* RT_USING_FINSH */
