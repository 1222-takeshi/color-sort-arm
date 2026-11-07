cmake_minimum_required(VERSION 3.16)
project(pan_tilt CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Host-side build of the dependency-free pan-tilt logic and its tests.
# The Pico firmware (firmware/pico) reuses pan_tilt.cpp unchanged.
add_library(pan_tilt pan_tilt.cpp)
target_include_directories(pan_tilt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_compile_options(pan_tilt PRIVATE -Wall -Wextra)

enable_testing()
add_executable(pan_tilt_tests test_pan_tilt.cpp)
target_link_libraries(pan_tilt_tests PRIVATE pan_tilt)
target_compile_options(pan_tilt_tests PRIVATE -Wall -Wextra)
add_test(NAME pan_tilt_tests COMMAND pan_tilt_tests)
