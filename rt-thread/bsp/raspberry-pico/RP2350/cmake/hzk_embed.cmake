# 编译时将 HZK 字模二进制嵌入固件
# 字模复制到 C:/hzk/（纯 ASCII 路径），再用 .incbin 生成固定符号名

function(_hzk_embed_one TARGET BIN_FILE SECTION START_SYM END_SYM LINK_ADDR COMPILE_DEF)
    if(NOT EXISTS "${BIN_FILE}")
        message(FATAL_ERROR "HZK 字模文件不存在: ${BIN_FILE}")
    endif()

    get_filename_component(_bin_name "${BIN_FILE}" NAME)
    set(_staging_dir "C:/hzk")
    set(_staging_bin "${_staging_dir}/${SECTION}.bin")
    set(_asm "${_staging_dir}/embed_${SECTION}.S")
    set(_obj "${_staging_dir}/embed_${SECTION}.o")

    file(MAKE_DIRECTORY "${_staging_dir}")

    # 生成汇编 incbin 源（路径固定为 C:/hzk/xxx.bin，符号名可预测）
    file(WRITE "${_asm}"
".section .rodata.${SECTION}, \"a\"
.global ${START_SYM}, ${END_SYM}
.align 4
${START_SYM}:
.incbin \"C:/hzk/${SECTION}.bin\"
${END_SYM}:
")

    add_custom_command(
        OUTPUT "${_obj}"
        COMMAND ${CMAKE_COMMAND} -E copy "${BIN_FILE}" "${_staging_bin}"
        COMMAND ${CMAKE_ASM_COMPILER} -c -march=armv8-m.main+fp+dsp -mcpu=cortex-m33 -mthumb
                -ffunction-sections -fdata-sections -mfloat-abi=softfp -mfpu=fpv5-sp-d16 -mcmse
                -x assembler-with-cpp -Wa,-mimplicit-it=always
                -o "${_obj}" "${_asm}"
        DEPENDS "${BIN_FILE}"
        COMMENT "嵌入字库 ${SECTION}: ${_bin_name}"
        VERBATIM
    )

    target_sources(${TARGET} PRIVATE "${_obj}")
    target_compile_definitions(${TARGET} PRIVATE ${COMPILE_DEF})

    if(LINK_ADDR AND NOT LINK_ADDR EQUAL 0)
        target_link_options(${TARGET} PRIVATE
            "LINKER:--section-start=.rodata.${SECTION}=${LINK_ADDR}")
        message(STATUS "HZK ${SECTION}: ${BIN_FILE} -> Flash 0x${LINK_ADDR}")
    else()
        message(STATUS "HZK ${SECTION}: ${BIN_FILE} -> 固件末尾（链接时自动分配地址）")
    endif()
endfunction()

function(hzk_embed_all TARGET)
    if(HZK16_SOURCE_FILE)
        _hzk_embed_one(${TARGET}
            "${HZK16_SOURCE_FILE}" hzk16 __hzk16_start __hzk16_end
            "${HZK16_LINK_ADDR}" HZK16_EMBEDDED)
    endif()

    if(HZK12_SOURCE_FILE)
        _hzk_embed_one(${TARGET}
            "${HZK12_SOURCE_FILE}" hzk12 __hzk12_start __hzk12_end
            "${HZK12_LINK_ADDR}" HZK12_EMBEDDED)
    endif()
endfunction()
