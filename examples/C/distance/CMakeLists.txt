# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseExamples-Distance)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(rs-distance rs-distance.c ../example.h)
include_directories(../../C)
target_link_libraries(rs-distance ${DEPENDENCIES})
set_target_properties (rs-distance PROPERTIES
FOLDER "Examples/C"
)

install(TARGETS rs-distance RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

