# HZK 字模嵌入配置（scons --target=cmake 不会覆盖此文件）

get_filename_component(HZK_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

# 字模原始文件路径（工程根目录下的 HZK16 / HZK12）
set(HZK16_SOURCE_FILE "${HZK_PROJECT_ROOT}/HZK16")
set(HZK12_SOURCE_FILE "${HZK_PROJECT_ROOT}/HZK12")

# Flash 链接地址
#   0       = 紧接主程序之后自动排列（推荐，bin 无填充间隙）
#   非 0    = 强制指定地址（如 0x10100000，bin 会包含中间填充）
set(HZK16_LINK_ADDR 0)
set(HZK12_LINK_ADDR 0)

# RP2350 GPIO 协处理器与 gpio_get(SIO) 不同步，统一关闭
add_compile_definitions(PICO_USE_GPIO_COPROCESSOR=0)

# 用本地 config 覆盖 SDK 默认 pico2（30 GPIO），强制 RP2350B / 48 GPIO
set(_RP2350B_PICO_CFG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/board/pico_cfg_override")
file(MAKE_DIRECTORY "${_RP2350B_PICO_CFG_DIR}/pico")
file(WRITE "${_RP2350B_PICO_CFG_DIR}/pico/config_autogen.h"
"// 本文件由 custom.cmake 生成，优先于 SDK pico_base_headers
#include \"boards/solderparty_rp2350_stamp_xl.h\"
#include \"cmsis/rename_exceptions.h\"
")
include_directories(BEFORE "${_RP2350B_PICO_CFG_DIR}")
add_compile_definitions(PICO_RP2350A=0)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/hzk_embed.cmake)
hzk_embed_all(${CMAKE_PROJECT_NAME}.elf)

# scons --target=cmake 偶发漏扫新文件，强制加入网络调试源
if(TARGET rtt_LVGL-net-tester)
    target_sources(rtt_LVGL-net-tester PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/applications/lvgl/net_tester/net_tester_netdbg.c
        ${CMAKE_CURRENT_SOURCE_DIR}/applications/lvgl/net_tester/net_tester_netops.c
    )
endif()
