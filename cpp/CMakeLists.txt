cmake_minimum_required(VERSION 3.12)

set(APP_NAME shapopad)
set(INC_DIR ./include)
set(SRC_DIR ./src)
set(SUBMODULE_DIR ../submodules)
set(LGFX_DIR ${SUBMODULE_DIR}/LovyanGFX)

# option(BOARD_PICO_W "Enable Pico W Functions" OFF) 
# option(WIFI_SSID "WiFi SSID" "") 
# option(WIFI_PASS "WiFi Pass Phrase" "") 

# Pull in PICO SDK (must be before project)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(${APP_NAME} C CXX ASM)

# Initialize the SDK
pico_sdk_init()

add_executable(${APP_NAME})

target_compile_options(${APP_NAME} PUBLIC -O2 -Wall)
target_compile_features(${APP_NAME} PUBLIC c_std_11)
target_compile_features(${APP_NAME} PUBLIC cxx_std_17)

# if(PICO_BOARD STREQUAL pico_w)
#     target_compile_definitions(${APP_NAME} PRIVATE
#         BOARD_PICO_W
#         ENABLE_NTP_SERVER
#         WIFI_SSID=\"$(WIFI_SSID)\"
#         WIFI_PASS=\"$(WIFI_PASS)\"
#     )
#     set(ADDITIONAL_LIBS
#         pico_cyw43_arch_lwip_poll
#     )
# endif()

# ${LGFX_DIR}/CMakeLists.txt を依存関係に加える
add_subdirectory(${LGFX_DIR} lgfx)

file(GLOB CPP_FILES
    ${SRC_DIR}/*.cpp
)

target_sources(${APP_NAME} PRIVATE 
    ${CPP_FILES}
)

target_include_directories(${APP_NAME} PRIVATE
    ${INC_DIR}
    ${LGFX_DIR}/src
)

target_link_libraries(${APP_NAME} PRIVATE
    pico_stdlib
    pico_multicore
    hardware_dma
    hardware_pwm
    hardware_adc
    hardware_spi
    hardware_i2c
    LovyanGFX
    ${ADDITIONAL_LIBS}
)

pico_enable_stdio_usb(${APP_NAME} 1)
pico_enable_stdio_uart(${APP_NAME} 0)

pico_add_extra_outputs(${APP_NAME})
