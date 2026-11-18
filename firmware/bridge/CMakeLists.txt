cmake_minimum_required(VERSION 3.16)
project(bridge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Host-side build of the bridge support logic and its tests. The Pico
# firmware (firmware/bridge_pico) reuses ring_buffer.h unchanged.
enable_testing()
add_executable(bridge_tests test_bridge.cpp)
target_include_directories(bridge_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_compile_options(bridge_tests PRIVATE -Wall -Wextra)
add_test(NAME bridge_tests COMMAND bridge_tests)
