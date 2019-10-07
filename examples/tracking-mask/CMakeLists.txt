# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesTrackingMask)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-tracking-mask rs-tracking-mask.cpp ../example.hpp)
    set_property(TARGET rs-tracking-mask PROPERTY CXX_STANDARD 11)
    target_include_directories(rs-tracking-mask PUBLIC ../)
    target_link_libraries(rs-tracking-mask ${DEPENDENCIES})
    set_target_properties (rs-tracking-mask PROPERTIES FOLDER Examples)
    install(TARGETS rs-tracking-mask RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
