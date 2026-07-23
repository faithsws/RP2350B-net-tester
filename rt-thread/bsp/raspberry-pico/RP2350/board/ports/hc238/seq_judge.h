/*
 * 测序判定：本端 H 通道 → 对端标识电阻通道
 */
#ifndef __SEQ_JUDGE_H__
#define __SEQ_JUDGE_H__

#include "pair_scan.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEQ_MAP_NONE  (-1)

typedef struct
{
    /* map_to[h] = 对端通道 0~7；SEQ_MAP_NONE 表示未匹配 */
    int8_t map_to[PAIR_CH_COUNT];
    /* 均值电阻，单位 0.001kΩ（便于调试打印） */
    int32_t mean_r_mk[PAIR_CH_COUNT];
} seq_scan_result_t;

void seq_judge(const pair_volt_cube_t *volt, seq_scan_result_t *out);
void seq_judge_log(const seq_scan_result_t *r);

#ifdef __cplusplus
}
#endif

#endif /* __SEQ_JUDGE_H__ */
