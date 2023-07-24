# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

project( realdds )

add_library( ${PROJECT_NAME} STATIC "" )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Library )

target_include_directories( ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# We, and all our dependents, need these:
target_link_libraries( ${PROJECT_NAME}
    PUBLIC rsutils dds )  # dds=FastDDS


# Headers -----------------------------------------------------------------------------------
#
set( REALDDS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/realdds" )
file(GLOB_RECURSE REALDDS_HEADER_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${REALDDS_INCLUDE_DIR}/*.h"
)
target_sources( ${PROJECT_NAME} PRIVATE ${REALDDS_HEADER_FILES} )
source_group(
    TREE ${REALDDS_INCLUDE_DIR}
    PREFIX "Header Files"
    FILES ${REALDDS_HEADER_FILES} )


# Sources -----------------------------------------------------------------------------------
#
set( REALDDS_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
file(GLOB_RECURSE REALDDS_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    "${REALDDS_SRC_DIR}/*"
)
target_sources( ${PROJECT_NAME} PRIVATE ${REALDDS_SOURCE_FILES} )
source_group(
    TREE ${REALDDS_SRC_DIR}
    PREFIX "Source Files"
    FILES ${REALDDS_SOURCE_FILES} )


# Install -----------------------------------------------------------------------------------
#
install( TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )


# Python Wrapper ---------------------------------------------------------------------------
#
if (BUILD_PYTHON_BINDINGS)
    add_subdirectory(py)
endif()

