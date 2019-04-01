# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 2.8)
project(libtm)

set(CMAKE_CXX_STANDARD 11)

set(LIBTM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBTM_INCLUDE_DIR "${LIBTM_ROOT}/libtm/include")
set(INFRA_INCLUDE_DIR "${LIBTM_ROOT}/libtm/src/infra")
set(LIBTM_SRC_DIR "${LIBTM_ROOT}/libtm/src")
set(LIBTM_RESOURCES_DIR "${LIBTM_ROOT}/resources")

set(HOST_VERSION "0.19.3.1711")

add_definitions(-DBUILD_STATIC)

# Build resources (FW, Central, Controller binaries)
add_subdirectory(fw)
add_subdirectory(libtm)
add_subdirectory(tools)
