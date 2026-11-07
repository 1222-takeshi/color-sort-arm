cmake_minimum_required(VERSION 3.13)

# Build with the Pico SDK: set PICO_SDK_PATH to the SDK checkout, e.g.
#   PICO_SDK_PATH=~/pico/pico-sdk cmake -S . -B build && cmake --build build
if(NOT DEFINED ENV{PICO_SDK_PATH})
  message(FATAL_ERROR "Set PICO_SDK_PATH to the Pico SDK checkout")
endif()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pan_tilt_pico C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(pan_tilt_pico main.cpp ../pan_tilt/pan_tilt.cpp)
target_include_directories(pan_tilt_pico PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(pan_tilt_pico pico_stdlib hardware_i2c)
pico_add_extra_outputs(pan_tilt_pico)
