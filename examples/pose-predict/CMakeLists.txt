# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesPosePredict)

if(BUILD_EXAMPLES)
    add_executable(rs-pose-predict rs-pose-predict.cpp)
    set_property(TARGET rs-pose-predict PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-pose-predict ${DEPENDENCIES})
    set_target_properties (rs-pose-predict PROPERTIES FOLDER Examples)
    install(TARGETS rs-pose-predict RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
