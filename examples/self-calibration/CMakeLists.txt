# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesSelfCalibration)

add_executable(rs-self-calibration self-calibration.cpp)
set_property(TARGET rs-self-calibration PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-self-calibration ${DEPENDENCIES})
include_directories(rs-self-calibration ../)
set_target_properties (rs-self-calibration PROPERTIES FOLDER "Examples")

install(TARGETS rs-self-calibration RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
