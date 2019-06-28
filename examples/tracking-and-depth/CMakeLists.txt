# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesTrackingAndDepth)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-tracking-and-depth rs-tracking-and-depth.cpp ../example.hpp)
    set_property(TARGET rs-tracking-and-depth PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-tracking-and-depth ${DEPENDENCIES})
    include_directories(rs-tracking-and-depth ../)
    set_target_properties (rs-tracking-and-depth PROPERTIES FOLDER Examples)
    install(TARGETS rs-tracking-and-depth RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    add_custom_command(
            TARGET rs-tracking-and-depth POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/H_t265_d400.cfg
                    ${CMAKE_CURRENT_BINARY_DIR}/H_t265_d400.cfg)
endif()
