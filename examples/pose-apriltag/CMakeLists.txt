# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)
project(rs-pose-apriltag)

if(BUILD_EXAMPLES)

    find_path(APRILTAG_INC NAMES apriltag.h PATH_SUFFIXES apriltag)
    find_library(APRILTAG_LIB apriltag)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(apriltag DEFAULT_MSG APRILTAG_INC APRILTAG_LIB)
    if(APRILTAG_FOUND AND NOT TARGET apriltag)
        add_library(apriltag INTERFACE)
        target_include_directories(apriltag INTERFACE ${APRILTAG_INC})
        target_link_libraries(apriltag INTERFACE ${APRILTAG_LIB})
    endif()

    if(TARGET apriltag)
        add_executable(rs-pose-apriltag rs-pose-apriltag.cpp)
        set_property(TARGET rs-pose-apriltag PROPERTY CXX_STANDARD 11)
        target_link_libraries(rs-pose-apriltag ${DEPENDENCIES} apriltag)
        set_target_properties (rs-pose-apriltag PROPERTIES FOLDER Examples)
        install(TARGETS rs-pose-apriltag RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    else()
        message(STATUS "Unable to find apriltag library, skipping pose-apriltag example")
    endif()

endif()
