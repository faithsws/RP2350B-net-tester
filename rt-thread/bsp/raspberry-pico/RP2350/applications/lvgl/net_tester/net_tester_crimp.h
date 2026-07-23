#ifndef NET_TESTER_CRIMP_H
#define NET_TESTER_CRIMP_H

#include "seq_judge.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 测序结果回调（文件名保留 crimp，语义为测序）。
 */
typedef void (*net_tester_crimp_result_cb_t)(const seq_scan_result_t *result,
                                             void *user_data);

void net_tester_crimp_set_result_cb(net_tester_crimp_result_cb_t cb, void *user_data);

bool net_tester_crimp_start(void);
void net_tester_crimp_stop(void);
bool net_tester_crimp_is_waiting(void);

void net_tester_crimp_report_result(const seq_scan_result_t *result);
void net_tester_crimp_simulate_async_done(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_CRIMP_H */
