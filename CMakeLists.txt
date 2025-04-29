cmake_minimum_required(VERSION 3.13)

# Pull in Raspberry Pi Pico SDK (must be defined, e.g., via environment variable PICO_SDK_PATH)
include(pico_sdk_import.cmake)

project(pico_joystick_example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# --- Your Project Configuration ---
set(PROJECT_NAME pico_joystick_example)
# --------------------------------

# Add include directories
include_directories(include)

# Add source files
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/m5_unit_joystick.cpp 
)

# Link necessary Pico libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_i2c
)

# Enable USB stdio for printf output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
# Alternatively, enable UART stdio (choose one)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Add other build options if needed (e.g., UF2 generation)
pico_add_extra_outputs(${PROJECT_NAME}) 