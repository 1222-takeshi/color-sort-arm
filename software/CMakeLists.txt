cmake_minimum_required(VERSION 3.16)
project(color_sort_arm CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Dependency-free core library (ik + calibration).
add_library(arm_core core/ik.cpp core/calibration.cpp)
target_include_directories(arm_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(arm_core PRIVATE -Wall -Wextra)

# Vision library: the only part that needs OpenCV. Built only if found.
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
  add_library(arm_vision vision/color_detector.cpp)
  target_include_directories(arm_vision PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(arm_vision PUBLIC ${OpenCV_LIBS})
  target_compile_options(arm_vision PRIVATE -Wall -Wextra)
  message(STATUS "OpenCV found: building vision module")
else()
  message(STATUS "OpenCV not found: vision module and --image mode disabled")
endif()

# STS3215 packet protocol (dependency-free).
add_library(arm_proto proto/sts3215.cpp)
target_include_directories(arm_proto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(arm_proto PRIVATE -Wall -Wextra)

# Driver boundary (stub implementation).
add_library(arm_app app/arm_driver.cpp)
target_include_directories(arm_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(arm_app PUBLIC arm_core)
target_compile_options(arm_app PRIVATE -Wall -Wextra)

# Main application.
add_executable(color_sort app/main.cpp)
target_link_libraries(color_sort PRIVATE arm_core arm_app)
target_compile_options(color_sort PRIVATE -Wall -Wextra)
if(OpenCV_FOUND)
  target_link_libraries(color_sort PRIVATE arm_vision)
  target_compile_definitions(color_sort PRIVATE COLOR_SORT_WITH_OPENCV)
endif()

# Unit tests (no external dependencies).
enable_testing()
add_executable(arm_tests tests/test_core.cpp)
target_link_libraries(arm_tests PRIVATE arm_core)
target_compile_options(arm_tests PRIVATE -Wall -Wextra)
add_test(NAME arm_tests COMMAND arm_tests)

add_executable(proto_tests tests/test_proto.cpp)
target_link_libraries(proto_tests PRIVATE arm_proto)
target_compile_options(proto_tests PRIVATE -Wall -Wextra)
add_test(NAME proto_tests COMMAND proto_tests)
