# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesLabeledPointcloud)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-labeledpointcloud rs-labeledpointcloud.cpp ../example.hpp)
    set_property(TARGET rs-labeledpointcloud PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-labeledpointcloud ${DEPENDENCIES})
    include_directories(../)
    set_target_properties (rs-labeledpointcloud PROPERTIES FOLDER Examples)
    install(TARGETS rs-labeledpointcloud RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
