# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)
project(RealsenseTools)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

set(DEPENDENCIES ${DEPENDENCIES} realsense2)

add_subdirectory(convert)
add_subdirectory(enumerate-devices)
add_subdirectory(fw-logger)
add_subdirectory(terminal)
add_subdirectory(recorder)
add_subdirectory(fw-update)

if(NOT WIN32)
    if(BUILD_NETWORK_DEVICE)
        add_subdirectory(rs-server)
    endif()
endif()

if(BUILD_GRAPHICAL_EXAMPLES)
    include(${CMAKE_SOURCE_DIR}/CMake/opengl_config.cmake)
    if (NOT BUILD_GLSL_EXTENSIONS)
        MESSAGE(STATUS "BUILD_GRAPHICAL_EXAMPLES explicitely depends on BUILD_GLSL_EXTENSIONS, set it ON")
        SET(BUILD_GLSL_EXTENSIONS ON)
    endif()
    set(DEPENDENCIES ${DEPENDENCIES} realsense2-gl)
    add_subdirectory(data-collect)
    add_subdirectory(realsense-viewer)
    add_subdirectory(depth-quality)
    add_subdirectory(rosbag-inspector)
    add_subdirectory(benchmark)
else()
    if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
        find_library(log-lib log)
        add_dependencies(RealsenseTools log)
        set(DEPENDENCIES ${DEPENDENCIES} log)
    #else()
    #    set(DEPENDENCIES realsense2)
    endif()
endif()
