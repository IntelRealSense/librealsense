# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2026 RealSense, Inc. All Rights Reserved.
cmake_minimum_required(VERSION 3.10)

project(RealsenseExamples-CompositeCtl)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Same full get/set/get_range/metadata sequence as
# examples/composite-option's rs-composite-option.cpp, through the raw C API instead - no
# exceptions, no templates, no RAII. See rs-composite-ctl.c.
add_executable(rs-composite-ctl rs-composite-ctl.c ../example.h)
include_directories(../../C)
target_link_libraries(rs-composite-ctl ${DEPENDENCIES})
set_target_properties (rs-composite-ctl PROPERTIES
FOLDER "Examples/C"
)

install(TARGETS rs-composite-ctl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
