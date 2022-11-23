# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

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
    list( APPEND UTILITIES_HEADER_FILES
        "${REPO_ROOT}/third-party/easyloggingpp/src/easylogging++.h"
        )
endif()
target_sources( ${PROJECT_NAME} PRIVATE ${UTILITIES_HEADER_FILES} )


# Sources -----------------------------------------------------------------------------------
#
set( UTILITIES_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
file(GLOB_RECURSE UTILITIES_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${UTILITIES_SRC_DIR}/*"
)
if( BUILD_EASYLOGGINGPP )
    list( APPEND UTILITIES_SOURCE_FILES
        "${REPO_ROOT}/third-party/easyloggingpp/src/easylogging++.cc"
        )
endif()
target_sources( ${PROJECT_NAME} PRIVATE ${UTILITIES_SOURCE_FILES} )


# Install -----------------------------------------------------------------------------------
#
install( TARGETS ${PROJECT_NAME} EXPORT realsense2Targets)
