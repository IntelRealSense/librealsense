# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesAlignGl )

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_GRAPHICAL_EXAMPLES AND NOT APPLE)
    add_executable(rs-align-gl rs-align-gl.cpp ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp)
    set_property(TARGET rs-align-gl PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-align-gl ${DEPENDENCIES} realsense2-gl)
    include_directories(../../common ../../third-party/imgui ../../examples)
    set_target_properties (rs-align-gl PROPERTIES FOLDER Examples)
    install(TARGETS rs-align-gl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
