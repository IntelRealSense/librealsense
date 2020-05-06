# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamples)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

if(BUILD_GRAPHICAL_EXAMPLES)
    include(${CMAKE_SOURCE_DIR}/CMake/opengl_config.cmake)
else()
    if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
        find_library(log-lib log)
        set(DEPENDENCIES realsense2 log)
    else()
        set(DEPENDENCIES realsense2)
    endif()
endif()

add_subdirectory(hello-realsense)
add_subdirectory(software-device)
add_subdirectory(capture)
add_subdirectory(callback)
add_subdirectory(save-to-disk)
add_subdirectory(multicam)
add_subdirectory(pointcloud)
add_subdirectory(align)
add_subdirectory(align-advanced)
add_subdirectory(sensor-control)
add_subdirectory(measure)
add_subdirectory(C/depth)
add_subdirectory(C/color)
add_subdirectory(C/distance)
add_subdirectory(post-processing)
add_subdirectory(record-playback)
add_subdirectory(motion)
add_subdirectory(gl)
add_subdirectory(pose)
add_subdirectory(pose-predict)
add_subdirectory(pose-and-image)
add_subdirectory(trajectory)
add_subdirectory(ar-basic)
add_subdirectory(ar-advanced)
add_subdirectory(pose-apriltag)
add_subdirectory(tracking-and-depth)
