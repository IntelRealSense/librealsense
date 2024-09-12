# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsFirmwareLogger)

add_executable(rs-fw-logger rs-fw-logger.cpp)
set_property(TARGET rs-fw-logger PROPERTY CXX_STANDARD 11)
if(WIN32 OR ANDROID)
    target_link_libraries(rs-fw-logger ${DEPENDENCIES} tclap )
else()
    target_link_libraries(rs-fw-logger -lpthread ${DEPENDENCIES} tclap )
endif()
include_directories(../../common cpp-fw-logger-helpers)
set_target_properties (rs-fw-logger PROPERTIES
    FOLDER Tools
)

install(
    TARGETS

    rs-fw-logger

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

