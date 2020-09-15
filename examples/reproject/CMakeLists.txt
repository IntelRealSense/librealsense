# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesReproject)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-reproject rs-reproject.cpp ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp)
    set_property(TARGET rs-reproject PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-reproject ${DEPENDENCIES})
    include_directories(rs-reproject ../../common ../../third-party/imgui)
    set_target_properties (rs-reproject PROPERTIES FOLDER Examples)
    install(TARGETS rs-reproject RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
