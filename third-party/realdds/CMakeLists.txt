# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

project( realdds )

add_library( ${PROJECT_NAME} STATIC "" )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Library )

list( APPEND DEPENDENCIES utilities )

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "${REPO_ROOT}/include"  # we need the utilities
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries( ${PROJECT_NAME} PUBLIC ${DEPENDENCIES} dds )  # FastDDS


# Headers -----------------------------------------------------------------------------------
#
set( REALDDS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/realdds" )
file(GLOB_RECURSE REALDDS_HEADER_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${REALDDS_INCLUDE_DIR}/*.h"
)
target_sources( ${PROJECT_NAME} PRIVATE ${REALDDS_HEADER_FILES} )
file(GLOB_RECURSE REALDDS_TOPICS_HEADER_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${REALDDS_INCLUDE_DIR}/topics/*.h"
)
source_group( "Header Files/topics" FILES ${REALDDS_TOPICS_HEADER_FILES} )


# Sources -----------------------------------------------------------------------------------
#
set( REALDDS_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
file(GLOB_RECURSE REALDDS_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${REALDDS_SRC_DIR}/*"
)
target_sources( ${PROJECT_NAME} PRIVATE ${REALDDS_SOURCE_FILES} )
file(GLOB_RECURSE REALDDS_TOPICS_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${REALDDS_SRC_DIR}/topics/*"
)
source_group( "Source Files/topics" FILES ${REALDDS_TOPICS_SOURCE_FILES} )


# Install -----------------------------------------------------------------------------------
#
install( TARGETS realdds EXPORT realsense2Targets)


# Python Wrapper ---------------------------------------------------------------------------
#
if (BUILD_PYTHON_BINDINGS)
    add_subdirectory(py)
endif()

