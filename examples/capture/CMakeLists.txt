# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseExamplesCapture)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-capture rs-capture.cpp ../example.hpp ${IMGUI_SOURCES})
    set_property(TARGET rs-capture PROPERTY CXX_STANDARD 11)
    target_link_libraries( rs-capture ${DEPENDENCIES} tclap )
    include_directories(../ ../../examples)
    set_target_properties (rs-capture PROPERTIES FOLDER "Examples")

    install(TARGETS rs-capture RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
