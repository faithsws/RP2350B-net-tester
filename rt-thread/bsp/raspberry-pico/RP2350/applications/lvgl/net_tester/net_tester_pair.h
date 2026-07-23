#ifndef NET_TESTER_PAIR_H
#define NET_TESTER_PAIR_H

#include "pair_judge.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*net_tester_pair_result_cb_t)(const pair_scan_result_t *result,
                                            void *user_data);

void net_tester_pair_set_result_cb(net_tester_pair_result_cb_t cb, void *user_data);

bool net_tester_pair_start(void);
void net_tester_pair_stop(void);
bool net_tester_pair_is_waiting(void);

void net_tester_pair_report_result(const pair_scan_result_t *result);
void net_tester_pair_simulate_async_done(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_PAIR_H */
