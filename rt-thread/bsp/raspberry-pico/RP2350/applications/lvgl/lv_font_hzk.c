/*
 * HZK16 / HZK12 / HZK24(HZK12×2) -> LVGL v8 自定义字体
 *
 * 链路：源码 UTF-8 → Unicode → uni2gb → GB2312 区位取模。
 *
 * HZK 点阵：横向扫描、MSB 在左；每行字节对齐到 8bit。
 * - HZK16：16x16，每行 2 字节
 * - HZK12：12x12，每行 2 字节（后 4bit 空）；LVGL 需 box_w=16 才能对齐
 * - HZK24：由 HZK12 最近邻 2× 放大，24x24，每行 3 字节，box_w=24
 */
#include "lv_font_hzk.h"
#include "hzk.h"
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(lv_font_montserrat_14)
LV_FONT_DECLARE(lv_font_montserrat_20)

#define HZK24_WIDTH   24
#define HZK24_HEIGHT  24
#define HZK24_BYTES   72  /* 24 行 × 3 字节 */

typedef struct
{
    const hzk_source_t *src;
    uint8_t raw[HZK16_BYTES];
    uint8_t scaled[HZK24_BYTES];
    uint32_t cached_letter;
    uint8_t scale; /* 1=原样，2=最近邻放大到 24x24（仅 HZK12） */
} hzk_lv_font_dsc_t;

static hzk_lv_font_dsc_t hzk16_dsc;
static hzk_lv_font_dsc_t hzk12_dsc;
static hzk_lv_font_dsc_t hzk24_dsc;

static rt_bool_t hzk_font_source_ready(const hzk_source_t *src)
{
    if (src == RT_NULL)
    {
        return RT_FALSE;
    }

#ifdef RT_USING_DFS
    if (src->path != RT_NULL)
    {
        return RT_TRUE;
    }
#endif

    return src->data != RT_NULL;
}

/* HZK12 12x12 → 24x24 bpp=1 连续位图（最近邻 2×） */
static void hzk12_scale_x2(const uint8_t *src, uint8_t *dst)
{
    uint8_t y;
    uint8_t x;

    rt_memset(dst, 0, HZK24_BYTES);

    for (y = 0; y < HZK12_HEIGHT; y++)
    {
        for (x = 0; x < HZK12_WIDTH; x++)
        {
            uint8_t b = src[y * 2 + (x / 8)];
            uint8_t on = (uint8_t)((b >> (7 - (x % 8))) & 1);
            uint8_t dy;
            uint8_t dx;

            if (!on)
            {
                continue;
            }

            for (dy = 0; dy < 2; dy++)
            {
                for (dx = 0; dx < 2; dx++)
                {
                    uint8_t xx = (uint8_t)(x * 2 + dx);
                    uint8_t yy = (uint8_t)(y * 2 + dy);
                    dst[yy * 3 + (xx / 8)] |= (uint8_t)(0x80u >> (xx % 8));
                }
            }
        }
    }
}

static rt_bool_t hzk_load_letter(hzk_lv_font_dsc_t *dsc, uint32_t letter)
{
    uint16_t gb2312;

    if (!hzk_font_source_ready(dsc->src))
    {
        return RT_FALSE;
    }

    if (letter < 0x80)
    {
        return RT_FALSE;
    }

    if (letter == dsc->cached_letter)
    {
        return RT_TRUE;
    }

    if (!hzk_unicode_to_gb2312(letter, &gb2312))
    {
        return RT_FALSE;
    }

    if (!hzk_read_glyph(dsc->src, gb2312, dsc->raw, dsc->src->bytes_per_glyph))
    {
        return RT_FALSE;
    }

    if (dsc->scale == 2)
    {
        hzk12_scale_x2(dsc->raw, dsc->scaled);
    }

    dsc->cached_letter = letter;
    return RT_TRUE;
}

static bool hzk_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc_out,
                              uint32_t letter, uint32_t letter_next)
{
    hzk_lv_font_dsc_t *dsc = (hzk_lv_font_dsc_t *)font->dsc;
    uint8_t bit_w;

    RT_UNUSED(letter_next);

    if (dsc == RT_NULL || !hzk_load_letter(dsc, letter))
    {
        return false;
    }

    if (dsc->scale == 2)
    {
        dsc_out->adv_w = HZK24_WIDTH;
        dsc_out->box_w = HZK24_WIDTH;
        dsc_out->box_h = HZK24_HEIGHT;
    }
    else
    {
        /* HZK 行宽按字节对齐：12→16bit，16→16bit */
        bit_w = (uint8_t)(((dsc->src->width + 7) / 8) * 8);
        dsc_out->adv_w = dsc->src->width;
        dsc_out->box_w = bit_w;
        dsc_out->box_h = dsc->src->height;
    }

    dsc_out->ofs_x = 0;
    dsc_out->ofs_y = 0;
    dsc_out->bpp = 1;
    dsc_out->is_placeholder = 0;
    return true;
}

static const uint8_t *hzk_get_glyph_bitmap(const lv_font_t *font, uint32_t letter)
{
    hzk_lv_font_dsc_t *dsc = (hzk_lv_font_dsc_t *)font->dsc;

    if (dsc == RT_NULL || !hzk_load_letter(dsc, letter))
    {
        return RT_NULL;
    }

    return (dsc->scale == 2) ? dsc->scaled : dsc->raw;
}

lv_font_t lv_font_hzk16 = {
    .get_glyph_dsc = hzk_get_glyph_dsc,
    .get_glyph_bitmap = hzk_get_glyph_bitmap,
    .line_height = HZK16_HEIGHT,
    .base_line = 0,
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -2,
    .underline_thickness = 1,
    .dsc = &hzk16_dsc,
    .fallback = &lv_font_montserrat_14,
};

lv_font_t lv_font_hzk12 = {
    .get_glyph_dsc = hzk_get_glyph_dsc,
    .get_glyph_bitmap = hzk_get_glyph_bitmap,
    .line_height = HZK12_HEIGHT,
    .base_line = 0,
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -1,
    .underline_thickness = 1,
    .dsc = &hzk12_dsc,
    .fallback = &lv_font_montserrat_14,
};

lv_font_t lv_font_hzk24 = {
    .get_glyph_dsc = hzk_get_glyph_dsc,
    .get_glyph_bitmap = hzk_get_glyph_bitmap,
    .line_height = HZK24_HEIGHT,
    .base_line = 0,
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -2,
    .underline_thickness = 2,
    .dsc = &hzk24_dsc,
    .fallback = &lv_font_montserrat_20,
};

rt_err_t lv_font_hzk_init(void)
{
    hzk16_dsc.src = hzk16_get_source();
    hzk16_dsc.cached_letter = 0;
    hzk16_dsc.scale = 1;
    rt_memset(hzk16_dsc.raw, 0, sizeof(hzk16_dsc.raw));
    rt_memset(hzk16_dsc.scaled, 0, sizeof(hzk16_dsc.scaled));

    hzk12_dsc.src = hzk12_get_source();
    hzk12_dsc.cached_letter = 0;
    hzk12_dsc.scale = 1;
    rt_memset(hzk12_dsc.raw, 0, sizeof(hzk12_dsc.raw));
    rt_memset(hzk12_dsc.scaled, 0, sizeof(hzk12_dsc.scaled));

    /* 24x24：复用 HZK12 字模做 2× 放大，不额外占 Flash 字库 */
    hzk24_dsc.src = hzk12_get_source();
    hzk24_dsc.cached_letter = 0;
    hzk24_dsc.scale = 2;
    rt_memset(hzk24_dsc.raw, 0, sizeof(hzk24_dsc.raw));
    rt_memset(hzk24_dsc.scaled, 0, sizeof(hzk24_dsc.scaled));

    return RT_EOK;
}

/**
 * 从 Flash 字库读一字并打印点阵
 * @param label 说明文字
 * @param unicode BMP Unicode（如 0x7F51=网）
 */
static void hzk_dump_one(const char *label, uint32_t unicode)
{
    const hzk_source_t *s12 = hzk12_get_source();
    uint16_t gb = 0;
    uint8_t glyph[HZK12_BYTES];
    uint8_t y;
    uint8_t x;
    uint32_t offset;
    const uint8_t *flash_ptr;

    if (!hzk_unicode_to_gb2312(unicode, &gb))
    {
        rt_kprintf("[%s] U+%04X uni->gb FAIL\n", label, (unsigned)unicode);
        return;
    }

    offset = hzk_gb2312_to_offset(gb, HZK12_BYTES);
    flash_ptr = (s12 && s12->data && offset != 0xFFFFFFFF) ? (s12->data + offset) : RT_NULL;

    rt_kprintf("[%s] U+%04X GB=%04X off=%u flash=%p\n",
               label, (unsigned)unicode, gb, (unsigned)offset, flash_ptr);

    if (!hzk_read_glyph(s12, gb, glyph, HZK12_BYTES))
    {
        rt_kprintf("[%s] read_glyph FAIL\n", label);
        return;
    }

    if (flash_ptr)
    {
        int same = 1;
        for (x = 0; x < HZK12_BYTES; x++)
        {
            if (glyph[x] != flash_ptr[x])
            {
                same = 0;
                break;
            }
        }
        rt_kprintf("[%s] flash_vs_buf %s\n", label, same ? "MATCH" : "DIFF");
    }

    rt_kprintf("[%s] hex:", label);
    for (x = 0; x < HZK12_BYTES; x++)
    {
        rt_kprintf(" %02X", glyph[x]);
    }
    rt_kprintf("\n");

    rt_kprintf("[%s] bitmap MSB-horiz w12:\n", label);
    for (y = 0; y < HZK12_HEIGHT; y++)
    {
        for (x = 0; x < HZK12_WIDTH; x++)
        {
            uint8_t b = glyph[y * 2 + (x / 8)];
            uint8_t on = (uint8_t)((b >> (7 - (x % 8))) & 1);
            rt_kprintf("%c", on ? '#' : '.');
        }
        rt_kprintf("\n");
    }
}

/**
 * msh> hzk_check
 * msh> hzk_check 0x7F51
 */
static void hzk_check(int argc, char **argv)
{
    const hzk_source_t *s12 = hzk12_get_source();
    const hzk_source_t *s16 = hzk16_get_source();
    uint32_t unicode = 0x7F51;

    rt_kprintf("HZK12 data=%p size=%u w=%u h=%u bpg=%u\n",
               s12 ? s12->data : RT_NULL,
               s12 ? (unsigned)s12->size : 0,
               s12 ? s12->width : 0,
               s12 ? s12->height : 0,
               s12 ? s12->bytes_per_glyph : 0);
    rt_kprintf("HZK16 data=%p size=%u\n",
               s16 ? s16->data : RT_NULL,
               s16 ? (unsigned)s16->size : 0);
    rt_kprintf("HZK24 = HZK12 x2 software scale\n");

    if (argc >= 2)
    {
        unicode = (uint32_t)strtoul(argv[1], RT_NULL, 0);
    }

    hzk_dump_one("char", unicode);

    if (argc < 2)
    {
        hzk_dump_one("zhong", 0x4E2D);
        hzk_dump_one("yi", 0x4E00);
    }
}
MSH_CMD_EXPORT(hzk_check, dump HZK glyph: hzk_check [unicode]);
