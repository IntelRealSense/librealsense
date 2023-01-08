# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesPoseAndImage)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_EXAMPLES)
    add_executable(rs-pose-and-image rs-pose-and-image.cpp)
    set_property(TARGET rs-pose-and-image PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-pose-and-image ${DEPENDENCIES})
    include_directories(../../examples)
    set_target_properties (rs-pose-and-image PROPERTIES FOLDER Examples)
    install(TARGETS rs-pose-and-image RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
