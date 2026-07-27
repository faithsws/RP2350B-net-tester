/*
 * 网络巡线仪功能参数：INI 文本保存在 FAL "param" 分区
 *
 * 示例：
 *   # net-tester-param v1
 *   press.judge_thr_mv=30
 *   pair.tol_mv=150
 *   ...
 */
#ifndef __NET_PARAM_H__
#define __NET_PARAM_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 默认值（与原宏一致） ---- */
#define NET_PARAM_PRESS_JUDGE_THR_MV_DEFAULT   30u

#define NET_PARAM_PAIR_TOL_MV_DEFAULT          150u
#define NET_PARAM_PAIR_CONN_REMOTE_MV_DEFAULT  5500u
#define NET_PARAM_PAIR_CONN_H_MV_DEFAULT       6700u
#define NET_PARAM_PAIR_CONN_L_MV_DEFAULT       2600u
#define NET_PARAM_PAIR_SHORT_MH_HI_MV_DEFAULT  7300u
#define NET_PARAM_PAIR_SHORT_MH_MID_MV_DEFAULT 4700u
#define NET_PARAM_PAIR_SHORT_LH_LO_MV_DEFAULT  2000u
#define NET_PARAM_PAIR_SHORT_LH_MID_MV_DEFAULT 3600u

#define NET_PARAM_SEQ_MIN_MV_DEFAULT           50u

#define NET_PARAM_BAT_LVL4_MV_DEFAULT          4100u
#define NET_PARAM_BAT_LVL3_MV_DEFAULT          3900u
#define NET_PARAM_BAT_LVL2_MV_DEFAULT          3700u
#define NET_PARAM_BAT_LOW_MV_DEFAULT           3500u

typedef struct
{
    /* 压接 */
    rt_uint32_t press_judge_thr_mv;

    /* 对线判定 */
    rt_uint32_t pair_tol_mv;
    rt_uint32_t pair_conn_remote_mv;
    rt_uint32_t pair_conn_h_mv;
    rt_uint32_t pair_conn_l_mv;
    rt_uint32_t pair_short_mh_hi_mv;
    rt_uint32_t pair_short_mh_mid_mv;
    rt_uint32_t pair_short_lh_lo_mv;
    rt_uint32_t pair_short_lh_mid_mv;

    /* 测序：过低电压不参与电阻换算 */
    rt_uint32_t seq_min_mv;

    /* 电池电量档位 */
    rt_uint32_t bat_lvl4_mv;
    rt_uint32_t bat_lvl3_mv;
    rt_uint32_t bat_lvl2_mv;
    rt_uint32_t bat_low_mv;
} net_param_t;

const net_param_t *net_param_get(void);
void net_param_set_defaults(void);
rt_err_t net_param_load(void);
rt_err_t net_param_save(void);

/* 压接便捷接口（同步到 press_scan 运行时） */
rt_uint32_t net_param_press_judge_thr_mv(void);
void net_param_set_press_judge_thr_mv(rt_uint32_t mv);

#ifdef __cplusplus
}
#endif

#endif /* __NET_PARAM_H__ */
