# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsBenchmark)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-benchmark rs-benchmark.cpp ../../third-party/glad/glad.c)
    set_property(TARGET rs-benchmark PROPERTY CXX_STANDARD 11)
    target_link_libraries( rs-benchmark ${DEPENDENCIES} realsense2-gl tclap )
    include_directories( ../../third-party/glad ../../examples )
    set_target_properties (rs-benchmark PROPERTIES
        FOLDER Tools
    )

    install(
        TARGETS

        rs-benchmark

        RUNTIME DESTINATION
        ${CMAKE_INSTALL_BINDIR}
    )
endif()
