/*
 * HZK 字库 LVGL 字体驱动
 */
#ifndef __LV_FONT_HZK_H__
#define __LV_FONT_HZK_H__

#include <lvgl.h>
#include <rtthread.h>

extern lv_font_t lv_font_hzk16;
extern lv_font_t lv_font_hzk12;
extern lv_font_t lv_font_hzk24; /* HZK12 最近邻 2×，显示为 24x24 */

rt_err_t lv_font_hzk_init(void);

#endif /* __LV_FONT_HZK_H__ */
