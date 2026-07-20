/*
 * 网络测试仪 UI 入口（对接已有 ST7789 显示 + 按键/编码器）
 */
#include <lvgl.h>
#include <rtthread.h>
#include "lv_port_indev.h"
#include "lv_font_hzk.h"
#include "net_tester_ui.h"
#include "hzk12_font.h"

void lv_user_gui_init(void)
{
    lv_indev_t * keypad;
    lv_indev_t * encoder;

    if(lv_font_hzk_init() != RT_EOK) {
        rt_kprintf("[UI] HZK font init failed\n");
        return;
    }

    if(!hzk12_font_init()) {
        rt_kprintf("[UI] hzk12 wrapper init failed\n");
        return;
    }

    keypad = lv_port_indev_get_keypad();
    encoder = lv_port_indev_get_encoder();
    if(keypad == RT_NULL) {
        rt_kprintf("[UI] keypad indev missing\n");
        return;
    }

    net_tester_ui_init(keypad, encoder);
    rt_kprintf("[UI] net tester ready (240x240 ST7789)\n");
}
