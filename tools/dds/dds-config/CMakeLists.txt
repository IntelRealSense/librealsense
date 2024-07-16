# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required( VERSION 3.1.0 )
project( rs-dds-config )

add_executable( ${PROJECT_NAME} )

file( GLOB_RECURSE RS_DDS_CONFIG_SOURCE_FILES
    LIST_DIRECTORIES false
    RELATIVE ${PROJECT_SOURCE_DIR}
    "${CMAKE_CURRENT_LIST_DIR}/*"
    )
target_sources( ${PROJECT_NAME} PRIVATE ${RS_DDS_CONFIG_SOURCE_FILES} )

target_link_libraries( ${PROJECT_NAME} PRIVATE realdds realsense2 tclap )

set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER Tools/dds
    CXX_STANDARD 14
    )

install( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

using_easyloggingpp( ${PROJECT_NAME} SHARED )

