/*
 * HZK16 / HZK12 字模读取
 */
#include "hzk.h"
#include <string.h>

#ifdef RT_USING_DFS
#include <stdio.h>
#endif

static hzk_source_t hzk16_src;
static hzk_source_t hzk12_src;

uint32_t hzk_gb2312_to_offset(uint16_t gb2312, uint8_t bytes_per_glyph)
{
    uint8_t msb = (uint8_t)(gb2312 >> 8);
    uint8_t lsb = (uint8_t)(gb2312 & 0xFF);

    if (msb < 0xA1 || msb > 0xF7 || lsb < 0xA1 || lsb > 0xFE)
    {
        return 0xFFFFFFFF;
    }

    return (uint32_t)(94U * (msb - 0xA1) + (lsb - 0xA1)) * bytes_per_glyph;
}

static rt_bool_t hzk_read_from_mem(const hzk_source_t *src, uint32_t offset, uint8_t *out, uint32_t out_size)
{
    if (src == RT_NULL || src->data == RT_NULL || out == RT_NULL)
    {
        return RT_FALSE;
    }

    if (offset == 0xFFFFFFFF || (offset + out_size) > src->size)
    {
        return RT_FALSE;
    }

    rt_memcpy(out, src->data + offset, out_size);
    return RT_TRUE;
}

#ifdef RT_USING_DFS
static rt_bool_t hzk_read_from_file(const hzk_source_t *src, uint32_t offset, uint8_t *out, uint32_t out_size)
{
    FILE *fp;
    size_t read_size;

    if (src == RT_NULL || src->path == RT_NULL || out == RT_NULL)
    {
        return RT_FALSE;
    }

    if (offset == 0xFFFFFFFF)
    {
        return RT_FALSE;
    }

    fp = fopen(src->path, "rb");
    if (fp == RT_NULL)
    {
        return RT_FALSE;
    }

    if (fseek(fp, (long)offset, SEEK_SET) != 0)
    {
        fclose(fp);
        return RT_FALSE;
    }

    read_size = fread(out, 1, out_size, fp);
    fclose(fp);

    return read_size == out_size;
}
#endif

rt_bool_t hzk_read_glyph(const hzk_source_t *src, uint16_t gb2312, uint8_t *out, uint32_t out_size)
{
    uint32_t offset;

    if (src == RT_NULL || out == RT_NULL)
    {
        return RT_FALSE;
    }

    offset = hzk_gb2312_to_offset(gb2312, src->bytes_per_glyph);
#ifdef RT_USING_DFS
    if (src->path != RT_NULL)
    {
        return hzk_read_from_file(src, offset, out, out_size);
    }
#endif
    return hzk_read_from_mem(src, offset, out, out_size);
}

rt_err_t hzk16_set_source_mem(const uint8_t *data, uint32_t size)
{
    if (data == RT_NULL || size < HZK16_BYTES)
    {
        return -RT_EINVAL;
    }

    hzk16_src.data = data;
    hzk16_src.size = size;
    hzk16_src.width = HZK16_WIDTH;
    hzk16_src.height = HZK16_HEIGHT;
    hzk16_src.bytes_per_glyph = HZK16_BYTES;
#ifdef RT_USING_DFS
    hzk16_src.path = RT_NULL;
#endif
    return RT_EOK;
}

rt_err_t hzk12_set_source_mem(const uint8_t *data, uint32_t size)
{
    if (data == RT_NULL || size < HZK12_BYTES)
    {
        return -RT_EINVAL;
    }

    hzk12_src.data = data;
    hzk12_src.size = size;
    hzk12_src.width = HZK12_WIDTH;
    hzk12_src.height = HZK12_HEIGHT;
    hzk12_src.bytes_per_glyph = HZK12_BYTES;
#ifdef RT_USING_DFS
    hzk12_src.path = RT_NULL;
#endif
    return RT_EOK;
}

#ifdef RT_USING_DFS
rt_err_t hzk16_set_source_path(const char *path)
{
    if (path == RT_NULL)
    {
        return -RT_EINVAL;
    }

    hzk16_src.data = RT_NULL;
    hzk16_src.size = 0;
    hzk16_src.width = HZK16_WIDTH;
    hzk16_src.height = HZK16_HEIGHT;
    hzk16_src.bytes_per_glyph = HZK16_BYTES;
    hzk16_src.path = path;
    return RT_EOK;
}

rt_err_t hzk12_set_source_path(const char *path)
{
    if (path == RT_NULL)
    {
        return -RT_EINVAL;
    }

    hzk12_src.data = RT_NULL;
    hzk12_src.size = 0;
    hzk12_src.width = HZK12_WIDTH;
    hzk12_src.height = HZK12_HEIGHT;
    hzk12_src.bytes_per_glyph = HZK12_BYTES;
    hzk12_src.path = path;
    return RT_EOK;
}
#endif

const hzk_source_t *hzk16_get_source(void)
{
    return &hzk16_src;
}

const hzk_source_t *hzk12_get_source(void)
{
    return &hzk12_src;
}
