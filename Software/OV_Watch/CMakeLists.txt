cmake_minimum_required(VERSION 3.20)
project(OV_Watch C ASM)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/linker/STM32F411CEUx_APP.ld)
set(STARTUP_FILE ${CMAKE_CURRENT_LIST_DIR}/startup_stm32f411xe_gcc.s)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Core/Src/*.c
)

file(GLOB_RECURSE HAL_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/*.c
)

file(GLOB_RECURSE SYSTEM_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/SYSTEM/*.c
)

file(GLOB_RECURSE BSP_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/BSP/**/*.c
)

file(GLOB_RECURSE USER_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/User/**/*.c
)

file(GLOB LVGL_CORE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/core/*.c
)

file(GLOB LVGL_DRAW_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/draw/*.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/draw/sw/*.c
)

set(LVGL_FONT_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/extra/lv_extra.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/font/lv_font.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/font/lv_font_fmt_txt.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/font/lv_font_loader.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/font/lv_font_montserrat_12.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/font/lv_font_montserrat_14.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/font/lv_font_montserrat_16.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/font/lv_font_montserrat_18.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/font/lv_font_montserrat_20.c
)

file(GLOB LVGL_HAL_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/hal/*.c
)

file(GLOB LVGL_MISC_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/misc/*.c
)

file(GLOB LVGL_WIDGET_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/widgets/*.c
)

file(GLOB LVGL_EXTRA_THEME_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/extra/themes/default/*.c
)

file(GLOB LVGL_EXTRA_LAYOUT_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/extra/layouts/flex/*.c
)

file(GLOB LVGL_EXTRA_WIDGET_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/extra/widgets/calendar/*.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src/extra/widgets/meter/*.c
)

file(GLOB LVGL_PORT_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/porting/*.c
)

set(LVGL_SOURCES
  ${LVGL_CORE_SOURCES}
  ${LVGL_DRAW_SOURCES}
  ${LVGL_FONT_SOURCES}
  ${LVGL_HAL_SOURCES}
  ${LVGL_MISC_SOURCES}
  ${LVGL_WIDGET_SOURCES}
  ${LVGL_EXTRA_THEME_SOURCES}
  ${LVGL_EXTRA_LAYOUT_SOURCES}
  ${LVGL_EXTRA_WIDGET_SOURCES}
  ${LVGL_PORT_SOURCES}
)

set(FREERTOS_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/croutine.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/list.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/queue.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/tasks.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/timers.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2/cmsis_os2.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
)

set(EM7028_LIBS
  ${CMAKE_CURRENT_LIST_DIR}/BSP/EM7028/HeartRatelib.lib
  ${CMAKE_CURRENT_LIST_DIR}/BSP/EM7028/libBp.lib
  ${CMAKE_CURRENT_LIST_DIR}/BSP/EM7028/libBpm.lib
)

add_executable(${PROJECT_NAME}.elf
  ${CORE_SOURCES}
  ${HAL_SOURCES}
  ${SYSTEM_SOURCES}
  ${BSP_SOURCES}
  ${USER_SOURCES}
  ${LVGL_SOURCES}
  ${FREERTOS_SOURCES}
  ${STARTUP_FILE}
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/Core/Inc
  ${CMAKE_CURRENT_LIST_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
  ${CMAKE_CURRENT_LIST_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
  ${CMAKE_CURRENT_LIST_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
  ${CMAKE_CURRENT_LIST_DIR}/Drivers/CMSIS/Include
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/include
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl/src
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/lvgl
  ${CMAKE_CURRENT_LIST_DIR}/Middlewares/LVGL/GUI/porting
  ${CMAKE_CURRENT_LIST_DIR}/SYSTEM
  ${CMAKE_CURRENT_LIST_DIR}/BSP/LCD
  ${CMAKE_CURRENT_LIST_DIR}/BSP/AHT21
  ${CMAKE_CURRENT_LIST_DIR}/BSP/BL24C02
  ${CMAKE_CURRENT_LIST_DIR}/BSP/IIC
  ${CMAKE_CURRENT_LIST_DIR}/BSP/LSM303DLH
  ${CMAKE_CURRENT_LIST_DIR}/BSP/MPU6050
  ${CMAKE_CURRENT_LIST_DIR}/BSP/MPU6050/eMPL
  ${CMAKE_CURRENT_LIST_DIR}/BSP/OWDG
  ${CMAKE_CURRENT_LIST_DIR}/BSP/POWER
  ${CMAKE_CURRENT_LIST_DIR}/BSP/SPL06_001
  ${CMAKE_CURRENT_LIST_DIR}/BSP/EM7028
  ${CMAKE_CURRENT_LIST_DIR}/BSP/TOUCH
  ${CMAKE_CURRENT_LIST_DIR}/BSP/KEY
  ${CMAKE_CURRENT_LIST_DIR}/BSP/KT6328
  ${CMAKE_CURRENT_LIST_DIR}/User
  ${CMAKE_CURRENT_LIST_DIR}/User/GUI_App
  ${CMAKE_CURRENT_LIST_DIR}/User/GUI_App/Screens/Inc
  ${CMAKE_CURRENT_LIST_DIR}/User/Tasks/Inc
  ${CMAKE_CURRENT_LIST_DIR}/User/Func/Inc
)

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
  USE_HAL_DRIVER
  STM32F411xE
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=softfp
  -fshort-enums
  -fshort-wchar
  -ffunction-sections
  -fdata-sections
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wno-unused-variable
  -Wno-sign-compare
  -Wno-missing-field-initializers
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
  -T${LINKER_SCRIPT}
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=softfp
  -Wl,--gc-sections
  -Wl,-Map=${PROJECT_NAME}.map
  -specs=nosys.specs
  -specs=nano.specs
)

target_link_libraries(${PROJECT_NAME}.elf PRIVATE
  ${EM7028_LIBS}
  m
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
  COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
)
