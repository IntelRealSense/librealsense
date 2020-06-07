# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesTerminal)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-terminal-example rs-terminal-example.cpp ../example.hpp)
    set_property(TARGET rs-terminal-example PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-terminal-example ${DEPENDENCIES})
    include_directories(rs-terminal-example ../ ../../third-party/tclap/include)
    set_target_properties (rs-terminal-example PROPERTIES FOLDER "Examples")

    install(TARGETS rs-terminal-example RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
