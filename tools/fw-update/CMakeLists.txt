# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.

cmake_minimum_required(VERSION 3.1.0)

project(rs-fw-update)

add_executable(${PROJECT_NAME} rs-fw-update.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES})
include_directories(../../third-party/tclap/include ${CMAKE_BINARY_DIR})
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Tools)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
