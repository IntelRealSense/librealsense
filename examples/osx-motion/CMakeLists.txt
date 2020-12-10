# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(osx-motion)

find_library(CoreFoundationLib CoreFoundation)
find_library(IOKitLib IOKit)

add_executable(osx-motion main.c hidapi.c rs_hid.c)
target_link_libraries(osx-motion ${CoreFoundationLib} ${IOKitLib})

set_target_properties (osx-motion PROPERTIES FOLDER Examples)
install(TARGETS osx-motion RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
