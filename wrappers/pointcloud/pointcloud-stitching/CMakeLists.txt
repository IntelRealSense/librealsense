# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2021 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesPointcloudStitching)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(OpenGL)
if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "\n\n OpenGL package is missing!\n\n")
endif()

list(APPEND DEPENDENCIES ${OPENGL_LIBRARIES})
list(APPEND DEPENDENCIES glfw)

add_executable(rs-pointcloud-stitching rs-pointcloud-stitching.cpp rs-pointcloud-stitching.h ../../../examples/example.hpp ../../../third-party/imgui/imgui.cpp ../../../third-party/imgui/imgui_draw.cpp ../../../third-party/imgui/imgui_impl_glfw.cpp)
set_property(TARGET rs-pointcloud-stitching PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-pointcloud-stitching ${DEPENDENCIES} )
include_directories(./ ../../../examples ../../../third-party/tclap/include ../../../third-party/ ../../../third-party/imgui ../../../third-party/glfw/include ../../../common/res)
set_target_properties (rs-pointcloud-stitching PROPERTIES FOLDER "Examples")
install(TARGETS rs-pointcloud-stitching RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
