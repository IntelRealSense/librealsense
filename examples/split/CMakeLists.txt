# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesSplit)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-split rs-split.cpp ../example.hpp)
    set_property(TARGET rs-split PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-split ${DEPENDENCIES})
    include_directories(rs-split ../ ../../third-party/tclap/include)
    set_target_properties (rs-split PROPERTIES FOLDER "Examples")

    install(TARGETS rs-split RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
