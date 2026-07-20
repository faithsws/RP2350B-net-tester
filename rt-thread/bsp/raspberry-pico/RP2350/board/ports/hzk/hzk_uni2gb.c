/*
 * Unicode -> GB2312 对照表
 */
#include "hzk.h"

#include "hzk_uni2gb.inc"

rt_bool_t hzk_unicode_to_gb2312(uint32_t unicode, uint16_t *gb2312)
{
    int left = 0;
    int right = (int)HZK_UNI2GB_COUNT - 1;
    uint16_t key = (uint16_t)unicode;

    if (gb2312 == RT_NULL || unicode < 0x80)
    {
        return RT_FALSE;
    }

    while (left <= right)
    {
        int mid = left + ((right - left) >> 1);
        uint16_t cur = hzk_uni2gb_unicode[mid];

        if (cur == key)
        {
            *gb2312 = hzk_uni2gb_gb2312[mid];
            return RT_TRUE;
        }
        if (cur < key)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    return RT_FALSE;
}
