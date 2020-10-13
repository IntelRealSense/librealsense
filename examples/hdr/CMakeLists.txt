# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesHdr)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-hdr rs-hdr.cpp ../example.hpp)
    set_property(TARGET rs-hdr PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-hdr ${DEPENDENCIES})
    include_directories(rs-hdr ../ ../../third-party/tclap/include)
    set_target_properties (rs-hdr PROPERTIES FOLDER "Examples")

    install(TARGETS rs-hdr RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
