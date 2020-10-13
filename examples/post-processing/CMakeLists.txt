# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesPost-Processing)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-post-processing rs-post-processing.cpp ../example.hpp ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp)
    set_property(TARGET rs-post-processing PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-post-processing ${DEPENDENCIES})
    include_directories(rs-post-processing ../ ../../third-party/tclap/include ../../third-party/imgui)
    set_target_properties (rs-post-processing PROPERTIES FOLDER "Examples")
    install(TARGETS rs-post-processing RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
