#ifndef NET_TESTER_TRACE_H
#define NET_TESTER_TRACE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 开始寻线：输出 LINK_PWM 460kHz */
void net_tester_trace_start(void);

/* 停止寻线：关闭 LINK_PWM */
void net_tester_trace_stop(void);

bool net_tester_trace_is_running(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_TRACE_H */
