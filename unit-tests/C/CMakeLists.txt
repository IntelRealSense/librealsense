# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(unit-tests-rsutil-c)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(unit-tests-rsutil-c C/unit-tests-rsutil-c.c ../../include/librealsense2/rsutil.h)
include_directories(../C)
target_link_libraries(unit-tests-rsutil-c ${DEPENDENCIES})
set_target_properties (unit-tests-rsutil-c PROPERTIES
FOLDER "Unit-Tests"
)

install(TARGETS unit-tests-rsutil-c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
