cmake_minimum_required(VERSION 3.13)

# Pull in Raspberry Pi Pico SDK (must be defined, e.g., via environment variable PICO_SDK_PATH)
include(pico_sdk_import.cmake)

project(pico_joystick_example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/joystick
    ${CMAKE_CURRENT_SOURCE_DIR}/include/st7789
    ${PICO_SDK_PATH}/src/common/pico_stdlib/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_i2c/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include
)

# Common source files
set(COMMON_SOURCES
    src/joystick/joystick.cpp
)

# ST7789 source files
set(ST7789_SOURCES
    src/st7789/st7789.cpp
    src/st7789/st7789_hal.cpp
    src/st7789/st7789_gfx.cpp
    src/st7789/st7789_font.cpp
)

# Joystick test executable
add_executable(joystick_test
    examples/joystick_test.cpp
    ${COMMON_SOURCES}
)

# GameLauncher executable
add_executable(GameLauncher
    examples/GameLauncher.cpp
    ${COMMON_SOURCES}
    ${ST7789_SOURCES}
)

# PicoPilot game executable
add_executable(PicoPilot
    examples/PicoPilot.cpp
    ${COMMON_SOURCES}
    ${ST7789_SOURCES}
)

# CollisionX game executable
add_executable(CollisionX
    examples/CollisionX.cpp
    ${COMMON_SOURCES}
    ${ST7789_SOURCES}
)

# Link libraries for joystick_test
target_link_libraries(joystick_test
    pico_stdlib
    hardware_i2c
)

# Link libraries for GameLauncher
target_link_libraries(GameLauncher
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_dma
)

# Link libraries for PicoPilot
target_link_libraries(PicoPilot
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_dma
)

# Link libraries for CollisionX
target_link_libraries(CollisionX
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_dma
)

# Enable USB stdio for all executables
pico_enable_stdio_usb(joystick_test 1)
pico_enable_stdio_usb(GameLauncher 1)
pico_enable_stdio_usb(PicoPilot 1)
pico_enable_stdio_usb(CollisionX 1)

# Disable UART stdio for all executables
pico_enable_stdio_uart(joystick_test 0)
pico_enable_stdio_uart(GameLauncher 0)
pico_enable_stdio_uart(PicoPilot 0)
pico_enable_stdio_uart(CollisionX 0)

# Add other build options if needed (e.g., UF2 generation)
pico_add_extra_outputs(joystick_test)
pico_add_extra_outputs(GameLauncher)
pico_add_extra_outputs(PicoPilot)
pico_add_extra_outputs(CollisionX)
