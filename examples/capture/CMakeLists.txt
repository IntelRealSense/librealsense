# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesCapture)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-capture rs-capture.cpp ../example.hpp)
    set_property(TARGET rs-capture PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-capture ${DEPENDENCIES})
    include_directories(rs-capture ../ ../../third-party/tclap/include)
    set_target_properties (rs-capture PROPERTIES FOLDER "Examples")

    install(TARGETS rs-capture RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
