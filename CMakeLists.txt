cmake_minimum_required (VERSION 3.13)
include(pico_sdk_import.cmake)
set (BINARY "bearos") 
set (PROJ "bearos")
project ("bearos")

set (CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -Wconversion")
set (CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -Wconversion")

pico_sdk_init()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file (GLOB sdcard_src CONFIGURE_DEPENDS "drivers/sdcard/src/*.c")
file (GLOB i2c_lcd_src CONFIGURE_DEPENDS "drivers/i2c_lcd/src/*.c")
file (GLOB ds3231_src CONFIGURE_DEPENDS "drivers/ds3231/src/*.c")
file (GLOB waveshare_lcd_src CONFIGURE_DEPENDS "drivers/waveshare_lcd/src/*.c")
file (GLOB ff14a_src CONFIGURE_DEPENDS "fs/ff14a/source/*.c")
file (GLOB fatfs_sdcard_src CONFIGURE_DEPENDS "fs/fatfs_sdcard/src/*.c")
file (GLOB fatfs_loopback_src CONFIGURE_DEPENDS "fs/fatfs_loopback/src/*.c")
file (GLOB devfs_src CONFIGURE_DEPENDS "fs/devfs/src/*.c")
file (GLOB fat_src CONFIGURE_DEPENDS "fs/fat/src/*.c")
file (GLOB syslog_src CONFIGURE_DEPENDS "syslog/src/*.c")
file (GLOB sys_src CONFIGURE_DEPENDS "sys/src/*.c")
file (GLOB chardev_src CONFIGURE_DEPENDS "chardev/src/*.c")
file (GLOB term_src CONFIGURE_DEPENDS "term/src/*.c")
file (GLOB klib_src CONFIGURE_DEPENDS "klib/src/*.c")
file (GLOB shell_src CONFIGURE_DEPENDS "shell/src/*.c")
file (GLOB compat_src CONFIGURE_DEPENDS "compat/src/*.c")
file (GLOB devmgr_src CONFIGURE_DEPENDS "devmgr/src/*.c")
file (GLOB gfx_src CONFIGURE_DEPENDS "gfx/src/*.c")

add_executable(${BINARY}
    main.c ${sdcard_src} ${ff14a_src} ${fatfs_sdcard_src} ${fsmanager_src}
    ${syslog_src} ${fatfs_loopback_src} ${sys_src} ${chardev_src} 
    ${term_src} ${klib_src} ${shell_src} ${error_src} ${compat_src}
    ${i2c_lcd_src} ${devmgr_src} ${ds3231_src} ${fat_src} ${devfs_src}
    ${waveshare_lcd_src} ${gfx_src}
)
target_include_directories (${BINARY} PUBLIC drivers/i2c_lcd/include)
target_include_directories (${BINARY} PUBLIC drivers/ds3231/include)
target_include_directories (${BINARY} PUBLIC drivers/waveshare_lcd/include)
target_include_directories (${BINARY} PUBLIC drivers/sdcard/include)
target_include_directories (${BINARY} PUBLIC fs/fat/include)
target_include_directories (${BINARY} PUBLIC fs/devfs/include)
target_include_directories (${BINARY} PUBLIC fs/fatfs_sdcard/include)
target_include_directories (${BINARY} PUBLIC fs/fatfs_loopback/include)
target_include_directories (${BINARY} PUBLIC syslog/include)
target_include_directories (${BINARY} PUBLIC sys/include)
target_include_directories (${BINARY} PUBLIC chardev/include)
target_include_directories (${BINARY} PUBLIC fs/ff14a/source)
target_include_directories (${BINARY} PUBLIC term/include)
target_include_directories (${BINARY} PUBLIC klib/include)
target_include_directories (${BINARY} PUBLIC shell/include)
target_include_directories (${BINARY} PUBLIC compat/include)
target_include_directories (${BINARY} PUBLIC devmgr/include)
target_include_directories (${BINARY} PUBLIC api/bearos)
target_include_directories (${BINARY} PUBLIC api/include)
target_include_directories (${BINARY} PUBLIC gfx/include)

if (PICO_ON_DEVICE)
target_link_libraries (${BINARY} PRIVATE pico_stdlib hardware_spi hardware_dma hardware_rtc hardware_i2c )
else()
target_link_libraries (${BINARY} PRIVATE pico_stdlib )
endif()

pico_enable_stdio_usb (${BINARY} 1)
pico_enable_stdio_uart (${BINARY} 0)
pico_add_extra_outputs(${BINARY})

