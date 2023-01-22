# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesAR_Advanced)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-ar-advanced rs-ar-advanced.cpp ../example.hpp)
    set_property(TARGET rs-ar-advanced PROPERTY CXX_STANDARD 11)
    target_include_directories(rs-ar-advanced PUBLIC ../../examples)
    target_link_libraries(rs-ar-advanced ${DEPENDENCIES})
    set_target_properties (rs-ar-advanced PROPERTIES FOLDER Examples)
    install(TARGETS rs-ar-advanced RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
