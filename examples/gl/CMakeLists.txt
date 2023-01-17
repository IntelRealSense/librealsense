# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesGL)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_GRAPHICAL_EXAMPLES AND NOT APPLE)
    #pointcloud
    add_executable(rs-gl rs-gl.cpp ../example.hpp)
    set_property(TARGET rs-gl PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-gl ${DEPENDENCIES} realsense2-gl)
    include_directories(../)
    set_target_properties (rs-gl PROPERTIES FOLDER Examples)
    install(TARGETS rs-gl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
