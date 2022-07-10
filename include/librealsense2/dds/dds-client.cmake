# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set(DDS_CLIENT_FILES 
    "${CMAKE_CURRENT_LIST_DIR}/dds-defines.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-participant.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-participant.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dds-utilities.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device.cpp"
)

source_group( "DDS" FILES ${DDS_CLIENT_FILES} )
include("${REPO_ROOT}/include/librealsense2/dds/topics/dds-topics.cmake")

# NOTE: this requires that your "project(...)" statement match the executable
target_link_libraries( ${PROJECT_NAME} PRIVATE dds )
target_sources( ${PROJECT_NAME} PRIVATE ${DDS_CLIENT_FILES} )
