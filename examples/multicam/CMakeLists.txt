# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesMulticam)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-multicam rs-multicam.cpp ../example.hpp)
    set_property(TARGET rs-multicam PROPERTY CXX_STANDARD 11)
    include_directories(rs-multicam ../)
    target_link_libraries(rs-multicam ${DEPENDENCIES})
    set_target_properties (rs-multicam PROPERTIES FOLDER Examples)
    install(TARGETS rs-multicam RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
