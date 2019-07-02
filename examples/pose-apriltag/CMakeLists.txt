# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)
project(rs-pose-apriltag)

if(BUILD_EXAMPLES)

    #add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)
    find_library(apriltag-lib NAME apriltag PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

    if(TARGET apriltag)    
        add_executable(rs-pose-apriltag rs-pose-apriltag.cpp)
        set_property(TARGET rs-pose PROPERTY CXX_STANDARD 11)
        target_link_libraries(rs-pose-apriltag ${DEPENDENCIES} apriltag)
        set_target_properties (rs-pose-apriltag PROPERTIES FOLDER Examples)
        install(TARGETS rs-pose-apriltag RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()

endif()
