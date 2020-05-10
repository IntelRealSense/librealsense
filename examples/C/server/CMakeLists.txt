# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamples-Netserver)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(rs-netserver rs-netserver.c ../example.h)
target_link_libraries(rs-netserver realsense2-net ${DEPENDENCIES})
set_target_properties (rs-netserver PROPERTIES
FOLDER "Examples/C"
)

install(TARGETS rs-netserver RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

