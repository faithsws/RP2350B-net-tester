#ifndef NET_TESTER_PAIR_H
#define NET_TESTER_PAIR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 对线连通状态回调。
 * @param status  8 字节矩阵。status[i] 表示第 i+1 根线与其它线的连通：
 *                bit(j)=1 表示与第 j+1 根线联通（bit0=第1根）。
 *                例如 0b10101010 表示第1根与第2/4/6/8 根联通。
 * @param user_data 注册时传入的用户数据
 */
typedef void (*net_tester_pair_result_cb_t)(const uint8_t status[8], void * user_data);

/* 注册对线结果回调（硬件层后续调用 net_tester_pair_report_status 上报） */
void net_tester_pair_set_result_cb(net_tester_pair_result_cb_t cb, void * user_data);

/* 开始一次异步对线任务（调用 pair_scan，进入等待回调状态）
 * @return true 已进入等待；false 启动失败 */
bool net_tester_pair_start(void);

/* 停止/取消当前对线任务（离开界面时调用） */
void net_tester_pair_stop(void);

/* 是否正在等待异步结果 */
bool net_tester_pair_is_waiting(void);

/* 硬件上报连通状态：仅在等待中生效，打印日志并触发回调 */
void net_tester_pair_report_status(const uint8_t status[8]);

/* 测试用：模拟异步任务结束，随机生成连通状态并上报（对应空格键） */
void net_tester_pair_simulate_async_done(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_PAIR_H */
