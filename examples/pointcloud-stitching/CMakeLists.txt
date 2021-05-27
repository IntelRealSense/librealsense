# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesPointcloudStitching)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-pointcloud-stitching rs-pointcloud-stitching.cpp rs-pointcloud-stitching.h ../example.hpp ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp)
    set_property(TARGET rs-pointcloud-stitching PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-pointcloud-stitching ${DEPENDENCIES})
    include_directories(./ ../ ../../third-party/tclap/include ../../third-party/ ../../third-party/imgui ../../third-party/glfw/include ../../common/res)
    set_target_properties (rs-pointcloud-stitching PROPERTIES FOLDER "Examples")
    install(TARGETS rs-pointcloud-stitching RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
