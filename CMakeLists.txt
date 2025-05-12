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

# Joystick ST7789 integration executable
add_executable(joystick_st7789
    examples/joystick_st7789.cpp
    ${COMMON_SOURCES}
    ${ST7789_SOURCES}
)

# Little bees game executable
add_executable(little_bees
    examples/little_bees.cpp
    ${COMMON_SOURCES}
    ${ST7789_SOURCES}
)

# Link libraries for joystick_test
target_link_libraries(joystick_test
    pico_stdlib
    hardware_i2c
)

# Link libraries for joystick_st7789
target_link_libraries(joystick_st7789
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_dma
)

# Link libraries for little_bees
target_link_libraries(little_bees
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_dma
)

# Enable USB stdio for all executables
pico_enable_stdio_usb(joystick_test 1)
pico_enable_stdio_usb(joystick_st7789 1)
pico_enable_stdio_usb(little_bees 1)

# Disable UART stdio for all executables
pico_enable_stdio_uart(joystick_test 0)
pico_enable_stdio_uart(joystick_st7789 0)
pico_enable_stdio_uart(little_bees 0)

# Add other build options if needed (e.g., UF2 generation)
pico_add_extra_outputs(joystick_test)
pico_add_extra_outputs(joystick_st7789)
pico_add_extra_outputs(little_bees)
