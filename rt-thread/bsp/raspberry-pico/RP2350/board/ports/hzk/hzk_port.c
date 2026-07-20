/*
 * HZK 字库板级初始化
 *
 * 字模由 CMake custom.cmake / hzk_embed.cmake 以 .incbin 链入固件，
 * 符号 __hzk12_start/end、__hzk16_start/end 始终存在。
 * 注意：不要依赖 HZK12_EMBEDDED 宏——该宏只加在 elf 目标上，Drivers 库编译不到。
 */
#include <rtthread.h>
#include "hzk.h"
#include "lv_font_hzk.h"

extern const uint8_t __hzk16_start[];
extern const uint8_t __hzk16_end[];
extern const uint8_t __hzk12_start[];
extern const uint8_t __hzk12_end[];

int hzk_port_init(void)
{
    uint32_t size16 = (uint32_t)(__hzk16_end - __hzk16_start);
    uint32_t size12 = (uint32_t)(__hzk12_end - __hzk12_start);

    if (size16 >= HZK16_BYTES &&
        hzk16_set_source_mem(__hzk16_start, size16) == RT_EOK)
    {
        rt_kprintf("HZK16 loaded @%p size=%u\n", __hzk16_start, (unsigned)size16);
    }
    else
    {
        rt_kprintf("HZK16 load failed size=%u\n", (unsigned)size16);
    }

    if (size12 >= HZK12_BYTES &&
        hzk12_set_source_mem(__hzk12_start, size12) == RT_EOK)
    {
        rt_kprintf("HZK12 loaded @%p size=%u\n", __hzk12_start, (unsigned)size12);
    }
    else
    {
        rt_kprintf("HZK12 load failed size=%u\n", (unsigned)size12);
    }

    lv_font_hzk_init();
    return RT_EOK;
}
INIT_COMPONENT_EXPORT(hzk_port_init);
