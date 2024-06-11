# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseToolsRecorder)

add_executable(rs-record rs-record.cpp)
set_property(TARGET rs-record PROPERTY CXX_STANDARD 11)
target_link_libraries( rs-record ${DEPENDENCIES} tclap )
include_directories(../../common)
set_target_properties (rs-record PROPERTIES
    FOLDER "Tools"
)

install(
    TARGETS

    rs-record

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

