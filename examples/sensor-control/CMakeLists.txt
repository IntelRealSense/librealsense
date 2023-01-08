# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesSensorControl)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-sensor-control api_how_to.h rs-sensor-control.cpp)
    set_property(TARGET rs-sensor-control PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-sensor-control ${DEPENDENCIES})
    set_target_properties (rs-sensor-control PROPERTIES FOLDER Examples)
    install(TARGETS rs-sensor-control RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
