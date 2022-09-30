# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set( DDS_SERVER_HEADER_FILES 
    "${CMAKE_CURRENT_LIST_DIR}/dds-defines.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-guid.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-participant.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-topic.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-topic-reader.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device-broadcaster.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device-server.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-utilities.h"
)
set( DDS_SERVER_FILES ${DDS_SERVER_HEADER_FILES} )
source_group( "DDS" FILES ${DDS_SERVER_FILES} )

# NOTE: this requires that your "project(...)" statement match the executable
target_link_libraries( ${PROJECT_NAME} PRIVATE dds )
target_sources( ${PROJECT_NAME} PRIVATE ${DDS_SERVER_FILES} )

include("${REPO_ROOT}/include/librealsense2/dds/topics/dds-topics.cmake")
include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")

