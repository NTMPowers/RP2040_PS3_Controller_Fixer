# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD waveshare_rp2040_zero CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(RP2040_PS3_Controller_Fixer C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Option to enable the startup button combo sequence
option(ENABLE_COMBO "Enable the startup button combo sequence (PS3HEN activation)" OFF)

add_executable(RP2040_PS3_Controller_Fixer
    src/main.c
    src/usb_descriptors.c
)

pico_generate_pio_header(RP2040_PS3_Controller_Fixer ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

target_include_directories(RP2040_PS3_Controller_Fixer PRIVATE src)

target_link_libraries(RP2040_PS3_Controller_Fixer
    pico_stdlib
    tinyusb_device
    tinyusb_board
	hardware_pio
	hardware_clocks
)

# Pass the ENABLE_COMBO flag to the compiler if enabled
if(ENABLE_COMBO)
    target_compile_definitions(RP2040_PS3_Controller_Fixer PRIVATE ENABLE_COMBO)
endif()

# Disable standard IO to prevent conflicts with our HID implementation
pico_enable_stdio_usb(RP2040_PS3_Controller_Fixer 0)
pico_enable_stdio_uart(RP2040_PS3_Controller_Fixer 0)

pico_add_extra_outputs(RP2040_PS3_Controller_Fixer)