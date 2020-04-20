# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsFirmwareLogger)

add_executable(rs-fw-logger rs-fw-logger.cpp fw-log-data.cpp
    fw-log-data.h
    fw-logs-formating-options.cpp
    fw-logs-formating-options.h
    fw-logs-parser.cpp
    fw-logs-parser.h
    fw-logs-xml-helper.cpp
    fw-logs-xml-helper.h
    string-formatter.cpp
    string-formatter.h)
set_property(TARGET rs-fw-logger PROPERTY CXX_STANDARD 11)
if(WIN32 OR ANDROID)
    target_link_libraries(rs-fw-logger ${DEPENDENCIES})
else()
    target_link_libraries(rs-fw-logger -lpthread ${DEPENDENCIES})
endif()
include_directories(rs-fw-logger ../../third-party/tclap/include cpp-fw-logger-helpers)
set_target_properties (rs-fw-logger PROPERTIES
    FOLDER Tools
)

install(
    TARGETS

    rs-fw-logger

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

