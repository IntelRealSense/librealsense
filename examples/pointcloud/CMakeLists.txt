# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseExamplesPointcloud)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-pointcloud rs-pointcloud.cpp ../example.hpp ${IMGUI_SOURCES})
    set_property(TARGET rs-pointcloud PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-pointcloud ${DEPENDENCIES})
    include_directories(../)
    set_target_properties (rs-pointcloud PROPERTIES FOLDER Examples)
    install(TARGETS rs-pointcloud RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
