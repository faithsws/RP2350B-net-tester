/*
 * LVGL 输入：GPIO 按键 + EC11 编码器
 *
 * 板级映射（低电平有效）：
 *   Esc/返回  = GPIO36
 *   Enter/确认 = GPIO39
 *   Tab/切换   = EC11 按键 GPIO35 → LV_KEY_NEXT
 *   编码器旋钮 = GPIO33/34 → enc_diff
 */
#include <lvgl.h>
#include <rtdevice.h>
#include <rtthread.h>
#include "lv_port_indev.h"
#include "ec11.h"

#define KEY_BACK_PIN  36
#define KEY_ENTER_PIN 39

/* 编码器一格机械档位约 4 个边沿，合成一格后再交给 LVGL */
#define EC11_STEPS_PER_DETENT 4

static lv_indev_t *keypad_indev;
static lv_indev_t *encoder_indev;
static lv_group_t *keypad_group;
static int16_t enc_frac_acc;
static lv_port_key_remap_cb_t key_remap_cb;

/* 边沿日志用：避免每帧刷屏 */
static rt_bool_t prev_back;
static rt_bool_t prev_enter;
static rt_bool_t prev_tab;

void lv_port_indev_set_key_remap_cb(lv_port_key_remap_cb_t cb)
{
    key_remap_cb = cb;
}

static uint32_t apply_key_remap(uint32_t key)
{
    if (key_remap_cb)
    {
        return key_remap_cb(key);
    }
    return key;
}

static rt_bool_t key_is_pressed(rt_base_t pin)
{
    return rt_pin_read(pin) == PIN_LOW;
}

static void keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    static uint32_t last_key = 0;
    rt_bool_t back;
    rt_bool_t enter;
    rt_bool_t tab;

    RT_UNUSED(drv);

    back = key_is_pressed(KEY_BACK_PIN);
    enter = key_is_pressed(KEY_ENTER_PIN);
    tab = ec11_key_pressed();

    if (back != prev_back)
    {
        rt_kprintf("[KEY] Esc/GPIO%d %s\n", KEY_BACK_PIN, back ? "DOWN" : "UP");
        prev_back = back;
    }
    if (enter != prev_enter)
    {
        rt_kprintf("[KEY] Enter/GPIO%d %s\n", KEY_ENTER_PIN, enter ? "DOWN" : "UP");
        prev_enter = enter;
    }
    if (tab != prev_tab)
    {
        rt_kprintf("[KEY] Tab/GPIO%d %s\n", EC11_PIN_KEY, tab ? "DOWN" : "UP");
        prev_tab = tab;
    }

    if (back)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        last_key = apply_key_remap(LV_KEY_ESC);
        data->key = last_key;
        return;
    }

    if (enter)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        last_key = apply_key_remap(LV_KEY_ENTER);
        data->key = last_key;
        return;
    }

    if (tab)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        /* LVGL v8 KEYPAD 会把 LV_KEY_NEXT 固定用于切焦点；编辑态需 remap 成内部键 */
        last_key = apply_key_remap(LV_KEY_NEXT);
        data->key = last_key;
        return;
    }

    data->state = LV_INDEV_STATE_RELEASED;
    data->key = last_key;
}

static void encoder_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    int16_t raw;
    int16_t detent = 0;

    RT_UNUSED(drv);

    /* A/B 边沿中断已累加；此处只取差并合成“一格机械档位” */
    raw = ec11_take_diff();
    enc_frac_acc += raw;

    while (enc_frac_acc >= EC11_STEPS_PER_DETENT)
    {
        enc_frac_acc -= EC11_STEPS_PER_DETENT;
        detent++;
    }
    while (enc_frac_acc <= -EC11_STEPS_PER_DETENT)
    {
        enc_frac_acc += EC11_STEPS_PER_DETENT;
        detent--;
    }

    if (detent != 0)
    {
        rt_kprintf("[ENC] raw=%d detent=%d frac=%d A=%d B=%d\n",
                   (int)raw, (int)detent, (int)enc_frac_acc,
                   (int)rt_pin_read(EC11_PIN_A),
                   (int)rt_pin_read(EC11_PIN_B));
    }

    data->enc_diff = detent;
    /* 编码器按键走 keypad(LV_KEY_NEXT)，此处只报告释放 */
    data->state = LV_INDEV_STATE_RELEASED;
}

void lv_port_indev_init(void)
{
    static lv_indev_drv_t keypad_drv;
    static lv_indev_drv_t encoder_drv;
    lv_group_t *group;

    rt_pin_mode(KEY_BACK_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_mode(KEY_ENTER_PIN, PIN_MODE_INPUT_PULLUP);
    ec11_init();
    enc_frac_acc = 0;
    prev_back = key_is_pressed(KEY_BACK_PIN);
    prev_enter = key_is_pressed(KEY_ENTER_PIN);
    prev_tab = ec11_key_pressed();

    group = lv_group_create();
    lv_group_set_default(group);
    keypad_group = group;

    lv_indev_drv_init(&keypad_drv);
    keypad_drv.type = LV_INDEV_TYPE_KEYPAD;
    keypad_drv.read_cb = keypad_read;
    keypad_indev = lv_indev_drv_register(&keypad_drv);
    lv_indev_set_group(keypad_indev, group);

    lv_indev_drv_init(&encoder_drv);
    encoder_drv.type = LV_INDEV_TYPE_ENCODER;
    encoder_drv.read_cb = encoder_read;
    encoder_indev = lv_indev_drv_register(&encoder_drv);
    lv_indev_set_group(encoder_indev, group);
}

lv_group_t *lv_port_indev_get_group(void)
{
    return keypad_group;
}

lv_indev_t *lv_port_indev_get_keypad(void)
{
    return keypad_indev;
}

lv_indev_t *lv_port_indev_get_encoder(void)
{
    return encoder_indev;
}
