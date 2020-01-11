# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsDataCollect)

add_executable(rs-data-collect rs-data-collect.h rs-data-collect.cpp)
set_property(TARGET rs-data-collect PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-data-collect ${DEPENDENCIES})
include_directories(rs-data-collect ../../common ../../third-party/tclap/include)
set_target_properties (rs-data-collect PROPERTIES
    FOLDER "Tools"
)

install(
    TARGETS

    rs-data-collect

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

