#ifndef NET_TESTER_PRESS_H
#define NET_TESTER_PRESS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 压接结果回调。
 * @param status  1 字节位图。bit(i)=1 表示第 i+1 根线压接牢固。
 */
typedef void (*net_tester_press_result_cb_t)(uint8_t status, void *user_data);

void net_tester_press_set_result_cb(net_tester_press_result_cb_t cb, void *user_data);

void net_tester_press_start(void);
void net_tester_press_stop(void);
bool net_tester_press_is_waiting(void);

void net_tester_press_report_status(uint8_t status);
void net_tester_press_simulate_async_done(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_PRESS_H */
