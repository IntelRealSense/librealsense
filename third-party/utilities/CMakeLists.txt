# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8.0)  # source_group(TREE)
project( utilities )

add_library( ${PROJECT_NAME} STATIC "" )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Library )

target_include_directories( ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Headers -----------------------------------------------------------------------------------
#
set( UTILITIES_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/utilities" )
file(GLOB_RECURSE UTILITIES_HEADER_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${UTILITIES_INCLUDE_DIR}/*.h"
)
if( BUILD_EASYLOGGINGPP )
    target_sources( ${PROJECT_NAME}
        PRIVATE "${REPO_ROOT}/third-party/easyloggingpp/src/easylogging++.h" )
endif()
target_sources( ${PROJECT_NAME} PRIVATE ${UTILITIES_HEADER_FILES} )
source_group(
    TREE ${UTILITIES_INCLUDE_DIR}
    PREFIX "Header Files"
    FILES ${UTILITIES_HEADER_FILES} )


# Sources -----------------------------------------------------------------------------------
#
set( UTILITIES_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
file(GLOB_RECURSE UTILITIES_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${UTILITIES_SRC_DIR}/*"
)
if( BUILD_EASYLOGGINGPP )
    target_sources( ${PROJECT_NAME}
        PRIVATE "${REPO_ROOT}/third-party/easyloggingpp/src/easylogging++.cc" )
    # EasyLogging++ initialization needs to happen in any client of ours, or else
    # they will not have a working instance. This .cpp file will be automatically
    # added to any dependent target, but the initialization can be disabled with
    # NO_ELPP_INIT in the target makefile, e.g.:
    #     target_compile_definitions( ${PROJECT_NAME} PRIVATE NO_ELPP_INIT )
    target_sources( ${PROJECT_NAME}
        PUBLIC $<BUILD_INTERFACE:${UTILITIES_INCLUDE_DIR}/easylogging/elpp-init.cpp> )
endif()
target_sources( ${PROJECT_NAME} PRIVATE ${UTILITIES_SOURCE_FILES} )
source_group(
    TREE ${UTILITIES_SRC_DIR}
    PREFIX "Source Files"
    FILES ${UTILITIES_SOURCE_FILES} )


# Install -----------------------------------------------------------------------------------
#
install( TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )


# Python ------------------------------------------------------------------------------------
#
if( BUILD_PYTHON_BINDINGS )
    add_subdirectory( py )
endif()

