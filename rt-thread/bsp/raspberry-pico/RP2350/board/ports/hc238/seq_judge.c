/*
 * 测序判定算法（与 scripts/seq_scan_analyze.py 一致）
 *
 * 对每个 H：取 L!=H 且 MUX==L 的 Vmux，
 * R(kΩ) = 7.4/Vmux - 2，求均值后匹配对端标识电阻 ±0.2kΩ。
 *
 * 内部用 0.001kΩ 定点数：R_mk = 7400000/v_mv - 2000
 */
#include "seq_judge.h"
#include "net_param.h"

#include <string.h>

/* 标识电阻：0.5/1/1.5/2/2.5/3/3.9/4.7 kΩ → 0.001kΩ */
static const int32_t s_id_r_mk[PAIR_CH_COUNT] = {
    500, 1000, 1500, 2000, 2500, 3000, 3900, 4700
};
#define SEQ_R_TOL_MK      200
/* R_mk = SEQ_R_NUM_MK / v_mv - SEQ_R_OFF_MK ，对应 R(kΩ)=7.4/V - 2 */
#define SEQ_R_NUM_MK      7400000u
#define SEQ_R_OFF_MK      2000

static int32_t seq_r_mk_from_mv(uint32_t v_mv)
{
    uint32_t min_mv = net_param_get()->seq_min_mv;

    if (v_mv == PAIR_SCAN_INVALID_MV || v_mv < min_mv)
    {
        return -1;
    }
    return (int32_t)(SEQ_R_NUM_MK / v_mv) - SEQ_R_OFF_MK;
}

static int8_t seq_match_id(int32_t mean_mk)
{
    int i;
    int8_t best = SEQ_MAP_NONE;
    int32_t best_d = 0x7fffffff;

    for (i = 0; i < PAIR_CH_COUNT; i++)
    {
        int32_t d = mean_mk - s_id_r_mk[i];
        if (d < 0)
        {
            d = -d;
        }
        if (d <= SEQ_R_TOL_MK && d < best_d)
        {
            best_d = d;
            best = (int8_t)i;
        }
    }
    return best;
}

void seq_judge(const pair_volt_cube_t *volt, seq_scan_result_t *out)
{
    int h, l;

    if (!volt || !out)
    {
        return;
    }

    memset(out, 0, sizeof(*out));
    for (h = 0; h < PAIR_CH_COUNT; h++)
    {
        out->map_to[h] = SEQ_MAP_NONE;
        out->mean_r_mk[h] = 0;
    }

    for (h = 0; h < PAIR_CH_COUNT; h++)
    {
        int64_t sum = 0;
        int cnt = 0;

        for (l = 0; l < PAIR_CH_COUNT; l++)
        {
            int32_t r_mk;
            uint32_t v_mv;

            if (l == h)
            {
                continue;
            }
            /* 只取 MUX == L */
            v_mv = (*volt)[h][l][l];
            r_mk = seq_r_mk_from_mv(v_mv);
            if (r_mk < 0)
            {
                continue;
            }
            sum += r_mk;
            cnt++;
        }

        if (cnt <= 0)
        {
            continue;
        }
        out->mean_r_mk[h] = (int32_t)(sum / cnt);
        out->map_to[h] = seq_match_id(out->mean_r_mk[h]);
    }
}

void seq_judge_log(const seq_scan_result_t *r)
{
    int h;
    int ok = 0;
    int mapped = 0;

    if (!r)
    {
        return;
    }

    rt_kprintf("[SEQ_JUDGE] ---- 线序映射 ----\n");
    for (h = 0; h < PAIR_CH_COUNT; h++)
    {
        int8_t mid = r->map_to[h];
        int32_t mk = r->mean_r_mk[h];

        if (mid < 0)
        {
            rt_kprintf("[SEQ_JUDGE] 本端 ch%d -> ?  (mean= %d.%03dk)\n",
                       h, mk / 1000, (mk >= 0 ? mk : -mk) % 1000);
            continue;
        }
        mapped++;
        if (mid == h)
        {
            ok++;
        }
        rt_kprintf("[SEQ_JUDGE] 本端 ch%d -> ch%d  (mean=%d.%03dk %s)\n",
                   h, mid, mk / 1000, (mk >= 0 ? mk : -mk) % 1000,
                   (mid == h) ? "直通" : "错序");
    }
    rt_kprintf("[SEQ_JUDGE] 已匹配 %d/%d, 直通 %d/%d\n",
               mapped, PAIR_CH_COUNT, ok, PAIR_CH_COUNT);
}
