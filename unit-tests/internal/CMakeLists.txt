# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(internal-tests)

set (INTERNAL_TESTS_SOURCES
    internal-tests-main.cpp
    internal-tests-usb.cpp
    internal-tests-extrinsic.cpp
    internal-tests-types.cpp
    internal-tests-uv-map.cpp
    internal-tests-class-logic.cpp
)

add_executable(${PROJECT_NAME} ${INTERNAL_TESTS_SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES})
include_directories(${PROJECT_NAME} ../ ../../src/)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Unit-Tests")
