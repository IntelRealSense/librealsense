# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(test-utilities-rsutil-c)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
add_executable(test-utilities-rsutil-c test-rsutil-c.c ../../../include/librealsense2/rsutil.h)
# include_directories(../../../common)
target_link_libraries(test-utilities-rsutil-c ${DEPENDENCIES})
set_target_properties (test-utilities-rsutil-c PROPERTIES
FOLDER "API_C"
)

install(TARGETS test-utilities-rsutil-c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
