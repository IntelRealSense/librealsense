# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 20192024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseExamples-Depth)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(rs-depth rs-depth.c ../example.h)
include_directories(../../C)
target_link_libraries(rs-depth ${DEPENDENCIES})
set_target_properties (rs-depth PROPERTIES
FOLDER "Examples/C"
)

install(TARGETS rs-depth RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

