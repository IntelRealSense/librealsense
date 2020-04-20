# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsRecorder)

add_executable(rs-record rs-record.cpp)
set_property(TARGET rs-record PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-record ${DEPENDENCIES})
include_directories(rs-record ../../common ../../third-party/tclap/include)
set_target_properties (rs-record PROPERTIES
    FOLDER "Tools"
)

install(
    TARGETS

    rs-record

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

