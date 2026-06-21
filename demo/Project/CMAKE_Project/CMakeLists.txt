cmake_minimum_required(VERSION 3.20)

# =============================================================================
# Xiaomi AIoT Smart Plant Care Assistant - U1P (GD32F470)
# =============================================================================

project(PLANT_ASSISTANT C ASM)

# 导出 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# 可执行目标
# -----------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
    # 应用层
    ${CMAKE_CURRENT_SOURCE_DIR}/../Application/src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Application/src/plant_logic.c
    
    # BSP 驱动
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/systick.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/gd32f470z_eval.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/s1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/e1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/e2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/e3.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/c2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/src/uart.c
    
    # 固件库
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_rcu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_syscfg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_misc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Source/gd32f4xx_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/CMSIS/GD/GD32F4xx/Source/system_gd32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/startup_gd32f470_gcc.S
)

# -----------------------------------------------------------------------------
# 编译宏
# -----------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    USE_STDPERIPH_DRIVER
    GD32F450
)

# -----------------------------------------------------------------------------
# 头文件路径
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Application/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../GD32F470Z_BSP/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/GD32F4xx_standard_peripheral/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/CMSIS/GD/GD32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Library/Firmware/CMSIS
)

# -----------------------------------------------------------------------------
# 编译选项
# -----------------------------------------------------------------------------
target_compile_options(${PROJECT_NAME} PRIVATE
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wno-unused-function
    -Wno-unused-variable
    -O2
    $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp>
)

# -----------------------------------------------------------------------------
# 链接选项（使用本地链接脚本）
# -----------------------------------------------------------------------------
target_link_options(${PROJECT_NAME} PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/gd32f470zk.ld
    -Wl,--gc-sections
    -Wl,--no-warn-rwx-segments
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map,--cref
    -lc -lm -lnosys
    -u _printf_float
)

# -----------------------------------------------------------------------------
# 构建后：显示大小 + 生成 .hex / .bin
# -----------------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Firmware size:"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary -S $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMENT "Generated: ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
)