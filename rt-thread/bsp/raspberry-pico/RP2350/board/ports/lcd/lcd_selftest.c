/*
 * 纯 LCD 驱动自检（不依赖 LVGL）
 * finsh:
 *   lcd_test          - 彩色条纹 + 中心方块
 *   lcd_fill <color>  - 整屏纯色（0=黑 1=红 2=绿 3=蓝 4=白 5=黄）
 *   lcd_bl <0|1>      - 背光开关
 *
 * 定义 LCD_DRIVER_SELF_TEST=1 时启动自动跑一遍，并跳过 LVGL 线程以免覆盖。
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include "drv_lcd.h"

static uint16_t lcd_test_color_by_index(int idx)
{
    switch (idx)
    {
    case 0: return LCD_COLOR_BLACK;
    case 1: return LCD_COLOR_RED;
    case 2: return LCD_COLOR_GREEN;
    case 3: return LCD_COLOR_BLUE;
    case 4: return LCD_COLOR_WHITE;
    case 5: return LCD_COLOR_YELLOW;
    case 6: return LCD_COLOR_CYAN;
    case 7: return LCD_COLOR_MAGENTA;
    default: return LCD_COLOR_WHITE;
    }
}

static void lcd_draw_color_bars(void)
{
    const uint16_t palette[] = {
        LCD_COLOR_RED, LCD_COLOR_GREEN, LCD_COLOR_BLUE,
        LCD_COLOR_YELLOW, LCD_COLOR_CYAN, LCD_COLOR_MAGENTA,
        LCD_COLOR_WHITE, LCD_COLOR_BLACK
    };
    uint16_t bar_h;
    uint16_t i;

    bar_h = (uint16_t)(lcd_attr.height / 8);
    if (bar_h == 0)
    {
        bar_h = 1;
    }

    for (i = 0; i < 8; i++)
    {
        uint16_t y = (uint16_t)(i * bar_h);
        uint16_t h = (i == 7) ? (uint16_t)(lcd_attr.height - y) : bar_h;
        lcd_fill_rect(0, y, lcd_attr.width, h, palette[i]);
    }

    /* 中心对照块，确认坐标与字节序 */
    lcd_fill_rect(70, 70, 100, 100, LCD_COLOR_WHITE);
    lcd_fill_rect(90, 90, 60, 60, LCD_COLOR_RED);
}

static int lcd_test(int argc, char **argv)
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (lcd_hw_init() != RT_EOK)
    {
        rt_kprintf("[lcd_test] lcd_hw_init failed\n");
        return -1;
    }

    lcd_backlight_on();
    lcd_draw_color_bars();
    rt_kprintf("[lcd_test] color bars done (%dx%d)\n",
               lcd_attr.width, lcd_attr.height);
    return 0;
}
MSH_CMD_EXPORT(lcd_test, LCD driver color-bar self test);

static int lcd_fill(int argc, char **argv)
{
    int idx = 1;

    if (lcd_hw_init() != RT_EOK)
    {
        rt_kprintf("[lcd_fill] lcd_hw_init failed\n");
        return -1;
    }

    if (argc >= 2)
    {
        idx = atoi(argv[1]);
    }

    lcd_backlight_on();
    lcd_fill_color(lcd_test_color_by_index(idx));
    rt_kprintf("[lcd_fill] color_index=%d\n", idx);
    return 0;
}
MSH_CMD_EXPORT(lcd_fill, lcd_fill <0-7> fill solid color);

static int lcd_bl(int argc, char **argv)
{
    int on = 1;

    if (argc >= 2)
    {
        on = atoi(argv[1]);
    }

    if (on)
    {
        lcd_backlight_on();
        rt_kprintf("[lcd_bl] on\n");
    }
    else
    {
        lcd_backlight_off();
        rt_kprintf("[lcd_bl] off\n");
    }
    return 0;
}
MSH_CMD_EXPORT(lcd_bl, lcd_bl <0|1> backlight control);

#if LCD_DRIVER_SELF_TEST
static int lcd_selftest_boot(void)
{
    rt_kprintf("[lcd_test] SELF_TEST=1, run color bars (LVGL skipped)\n");
    lcd_test(0, RT_NULL);
    return 0;
}
INIT_APP_EXPORT(lcd_selftest_boot);
#endif
