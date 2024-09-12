# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project( rs-benchmark )

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_GRAPHICAL_EXAMPLES)

add_executable( ${PROJECT_NAME} rs-benchmark.cpp
    ../../third-party/glad/glad.c )
set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11 )
target_link_libraries( ${PROJECT_NAME} ${DEPENDENCIES} realsense2-gl tclap )
include_directories( ../../third-party/glad ../../examples )
set_target_properties( ${PROJECT_NAME} PROPERTIES
    FOLDER Tools
)

using_easyloggingpp( ${PROJECT_NAME} SHARED )

install(
    TARGETS

    ${PROJECT_NAME}

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

endif()
