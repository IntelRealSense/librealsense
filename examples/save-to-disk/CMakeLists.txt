# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseExamplesSaveToDisk)

add_executable(rs-save-to-disk rs-save-to-disk.cpp)
set_property(TARGET rs-save-to-disk PROPERTY CXX_STANDARD 11)
target_link_libraries( rs-save-to-disk ${DEPENDENCIES} tclap )
include_directories(../../common ../../third-party)
set_target_properties (rs-save-to-disk PROPERTIES FOLDER "Examples")
install(TARGETS rs-save-to-disk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
