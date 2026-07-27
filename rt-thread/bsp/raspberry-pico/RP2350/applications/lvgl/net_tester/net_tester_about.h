/*
 * 关于页：上部二维码（仅网址），底部明文显示版本/UID 与署名
 */
#ifndef NET_TESTER_ABOUT_H
#define NET_TESTER_ABOUT_H

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_TESTER_SW_VERSION  "1.0.0"
#define NET_TESTER_HW_VERSION  "1.0.0"
#define NET_TESTER_ABOUT_URL   "https://space.bilibili.com/627429493"

/* 创建关于屏幕 */
lv_obj_t * net_tester_about_create_screen(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_ABOUT_H */
