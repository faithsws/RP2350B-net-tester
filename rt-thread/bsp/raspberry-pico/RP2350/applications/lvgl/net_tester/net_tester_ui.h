#ifndef NET_TESTER_UI_H
#define NET_TESTER_UI_H

#include <lvgl.h>
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化网络测试仪界面（240x240，赛博朋克风格） */
void net_tester_ui_init(lv_indev_t * keypad, lv_indev_t * encoder);

#ifdef __cplusplus
}
#endif

#endif /* NET_TESTER_UI_H */
