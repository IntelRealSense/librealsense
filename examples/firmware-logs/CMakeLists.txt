# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesFirmwareLogs)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-firmware-logs rs-firmware-logs.cpp ../example.hpp)
    set_property(TARGET rs-firmware-logs PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-firmware-logs ${DEPENDENCIES})
    include_directories(rs-firmware-logs ../ ../../third-party/tclap/include)
    set_target_properties (rs-firmware-logs PROPERTIES FOLDER "Examples")

    install(TARGETS rs-firmware-logs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
