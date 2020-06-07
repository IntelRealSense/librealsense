# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesFirmwareLogs)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-flash-logs rs-flash-logs.cpp ../example.hpp)
    set_property(TARGET rs-flash-logs PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-flash-logs ${DEPENDENCIES})
    include_directories(rs-flash-logs ../ ../../third-party/tclap/include)
    set_target_properties (rs-flash-logs PROPERTIES FOLDER "Examples")

    install(TARGETS rs-flash-logs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
