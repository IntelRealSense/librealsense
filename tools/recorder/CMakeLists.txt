# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project( rs-record )

add_executable(${PROJECT_NAME} rs-record.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
target_link_libraries( ${PROJECT_NAME} ${DEPENDENCIES} tclap )
include_directories(../../common)
set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER "Tools"
)

using_easyloggingpp( ${PROJECT_NAME} SHARED )

install(
    TARGETS

    ${PROJECT_NAME}

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

