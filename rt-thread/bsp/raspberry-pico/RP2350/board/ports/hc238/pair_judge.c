/*
 * 对线判定算法（与 scripts/pair_scan_analyze.py 一致）
 * 电压阈值来自 net_param（Flash INI），缺省与原宏一致。
 */
#include "pair_judge.h"
#include "net_param.h"

#include <string.h>

static int pair_near_mv(uint32_t v_mv, uint32_t target_mv, uint32_t tol_mv)
{
    int d;

    if (v_mv == PAIR_SCAN_INVALID_MV)
    {
        return 0;
    }
    d = (int)v_mv - (int)target_mv;
    if (d < 0)
    {
        d = -d;
    }
    return d <= (int)tol_mv;
}

void pair_judge(const pair_volt_cube_t *volt, pair_scan_result_t *out)
{
    const net_param_t *p = net_param_get();
    rt_uint8_t h, l, m;
    uint32_t v;
    uint32_t tol;

    if (!volt || !out)
    {
        return;
    }

    tol = p->pair_tol_mv;
    memset(out, 0, sizeof(*out));
    for (h = 0; h < PAIR_CH_COUNT; h++)
    {
        out->ch_state[h] = PAIR_CH_OPEN;
    }

    for (h = 0; h < PAIR_CH_COUNT; h++)
    {
        for (l = 0; l < PAIR_CH_COUNT; l++)
        {
            if (h == l)
            {
                continue;
            }
            for (m = 0; m < PAIR_CH_COUNT; m++)
            {
                v = (*volt)[h][l][m];
                if (v == PAIR_SCAN_INVALID_MV)
                {
                    continue;
                }

                if (m == h && pair_near_mv(v, p->pair_conn_h_mv, tol))
                {
                    out->ch_state[h] = PAIR_CH_CONN;
                    out->ch_state[l] = PAIR_CH_CONN;
                    continue;
                }
                if (m == l && pair_near_mv(v, p->pair_conn_l_mv, tol))
                {
                    out->ch_state[h] = PAIR_CH_CONN;
                    out->ch_state[l] = PAIR_CH_CONN;
                    continue;
                }
                if (m != h && pair_near_mv(v, p->pair_conn_remote_mv, tol))
                {
                    out->ch_state[m] = PAIR_CH_CONN;
                    out->ch_state[h] = PAIR_CH_CONN;
                    continue;
                }
            }
        }
    }

    for (h = 0; h < PAIR_CH_COUNT; h++)
    {
        for (l = 0; l < PAIR_CH_COUNT; l++)
        {
            if (h == l)
            {
                continue;
            }
            for (m = 0; m < PAIR_CH_COUNT; m++)
            {
                v = (*volt)[h][l][m];
                if (v == PAIR_SCAN_INVALID_MV)
                {
                    continue;
                }

                if (pair_near_mv(v, p->pair_short_lh_lo_mv, tol) ||
                    pair_near_mv(v, p->pair_short_lh_mid_mv, tol))
                {
                    if (out->ch_state[h] == PAIR_CH_CONN &&
                        out->ch_state[l] == PAIR_CH_CONN)
                    {
                        out->ch_state[h] = PAIR_CH_SHORT;
                        out->ch_state[l] = PAIR_CH_SHORT;
                        out->short_bits[h] |= (uint8_t)(1u << l);
                        out->short_bits[l] |= (uint8_t)(1u << h);
                    }
                    continue;
                }

                if (m != h &&
                    (pair_near_mv(v, p->pair_short_mh_hi_mv, tol) ||
                     pair_near_mv(v, p->pair_short_mh_mid_mv, tol)))
                {
                    if (out->ch_state[m] == PAIR_CH_CONN &&
                        out->ch_state[h] == PAIR_CH_CONN)
                    {
                        out->ch_state[m] = PAIR_CH_SHORT;
                        out->ch_state[h] = PAIR_CH_SHORT;
                        out->short_bits[m] |= (uint8_t)(1u << h);
                        out->short_bits[h] |= (uint8_t)(1u << m);
                    }
                    continue;
                }
            }
        }
    }
}

void pair_judge_log(const pair_scan_result_t *r)
{
    static const char *names[] = { "断路", "联通", "短路" };
    int i, j;

    if (!r)
    {
        return;
    }

    rt_kprintf("[PAIR_JUDGE] ---- 通道状态 ----\n");
    for (i = 0; i < PAIR_CH_COUNT; i++)
    {
        uint8_t st = r->ch_state[i];
        if (st > PAIR_CH_SHORT)
        {
            st = PAIR_CH_OPEN;
        }
        rt_kprintf("[PAIR_JUDGE] 针脚%d(Y%d): %s\n", i + 1, i, names[st]);
    }

    rt_kprintf("[PAIR_JUDGE] ---- 短路对 ----\n");
    for (i = 0; i < PAIR_CH_COUNT; i++)
    {
        for (j = i + 1; j < PAIR_CH_COUNT; j++)
        {
            if (r->short_bits[i] & (1u << j))
            {
                rt_kprintf("[PAIR_JUDGE] 短路: %d-%d\n", i + 1, j + 1);
            }
        }
    }
}
