/*
 * 对线判定：断路 / 联通 / 短路
 */
#ifndef __PAIR_JUDGE_H__
#define __PAIR_JUDGE_H__

#include "pair_scan.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PAIR_CH_OPEN   0  /* 断路 */
#define PAIR_CH_CONN   1  /* 联通 */
#define PAIR_CH_SHORT  2  /* 短路 */

typedef struct
{
    uint8_t ch_state[PAIR_CH_COUNT];
    uint8_t short_bits[PAIR_CH_COUNT];
} pair_scan_result_t;

void pair_judge(const pair_volt_cube_t *volt, pair_scan_result_t *out);
void pair_judge_log(const pair_scan_result_t *r);

#ifdef __cplusplus
}
#endif

#endif /* __PAIR_JUDGE_H__ */
