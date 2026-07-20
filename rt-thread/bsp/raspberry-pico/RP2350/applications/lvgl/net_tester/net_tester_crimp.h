#ifndef NET_TESTER_CRIMP_H
#define NET_TESTER_CRIMP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 压接结果回调。
 * @param status  1 字节位图。bit(i)=1 表示第 i+1 根线压接牢固（bit0=第1根）。
 * @param user_data 注册时传入的用户数据
 */
typedef void (*net_tester_crimp_result_cb_t)(uint8_t status, void * user_data);

void net_tester_crimp_set_result_cb(net_tester_crimp_result_cb_t cb, void * user_data);

/* 开始一次异步压接检测 */
void net_tester_crimp_start(void);

/* 停止/取消当前等待 */
void net_tester_crimp_stop(void);

bool net_tester_crimp_is_waiting(void);

/* 硬件上报压接状态：仅在等待中生效 */
void net_tester_crimp_report_status(uint8_t status);

/* 测试用：空格模拟异步结束，随机上报压接位图 */
void net_tester_crimp_simulate_async_done(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_CRIMP_H */
