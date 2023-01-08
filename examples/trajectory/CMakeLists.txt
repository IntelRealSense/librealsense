# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesTrajectory)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-trajectory rs-trajectory.cpp ../example.hpp ../../common/res/t265.h ${LZ4_DIR}/lz4.c)
    set_property(TARGET rs-trajectory PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-trajectory ${DEPENDENCIES})
    include_directories(../ ../../common/res ${LZ4_DIR})
    set_target_properties (rs-trajectory PROPERTIES FOLDER Examples)
    install(TARGETS rs-trajectory RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
