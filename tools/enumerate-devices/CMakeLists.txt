# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseExamplesEnumerateDevices)

add_executable(rs-enumerate-devices rs-enumerate-devices.cpp)
set_property(TARGET rs-enumerate-devices PROPERTY CXX_STANDARD 11)
target_link_libraries( rs-enumerate-devices ${DEPENDENCIES} tclap )
set_target_properties (rs-enumerate-devices PROPERTIES
    FOLDER Tools
)

install(
    TARGETS

    rs-enumerate-devices

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
