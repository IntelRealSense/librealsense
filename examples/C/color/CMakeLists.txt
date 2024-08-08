# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseExamples-Color)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(rs-color rs-color.c ../example.h)
include_directories(../../common ../../third-party/imgui ../../C)
target_link_libraries(rs-color ${DEPENDENCIES})
set_target_properties (rs-color PROPERTIES
FOLDER "Examples/C"
)

install(TARGETS rs-color RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

