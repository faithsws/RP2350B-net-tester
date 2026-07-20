/*
 * HZK 点阵字库核心接口
 */
#ifndef __HZK_H__
#define __HZK_H__

#include <rtthread.h>
#include <stdint.h>

#define HZK16_WIDTH   16
#define HZK16_HEIGHT  16
#define HZK16_BYTES   32

#define HZK12_WIDTH   12
#define HZK12_HEIGHT  12
#define HZK12_BYTES   24

typedef struct
{
    const uint8_t *data;
    uint32_t size;
    uint8_t width;
    uint8_t height;
    uint8_t bytes_per_glyph;
#ifdef RT_USING_DFS
    const char *path;
#endif
} hzk_source_t;

rt_bool_t hzk_unicode_to_gb2312(uint32_t unicode, uint16_t *gb2312);
uint32_t hzk_gb2312_to_offset(uint16_t gb2312, uint8_t bytes_per_glyph);
rt_bool_t hzk_read_glyph(const hzk_source_t *src, uint16_t gb2312, uint8_t *out, uint32_t out_size);

rt_err_t hzk16_set_source_mem(const uint8_t *data, uint32_t size);
rt_err_t hzk12_set_source_mem(const uint8_t *data, uint32_t size);
#ifdef RT_USING_DFS
rt_err_t hzk16_set_source_path(const char *path);
rt_err_t hzk12_set_source_path(const char *path);
#endif
const hzk_source_t *hzk16_get_source(void);
const hzk_source_t *hzk12_get_source(void);

#endif /* __HZK_H__ */
