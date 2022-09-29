# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set( DDS_CLIENT_HEADER_FILES 
    "${CMAKE_CURRENT_LIST_DIR}/dds-defines.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-guid.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-participant.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-topic.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-topic-reader.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-utilities.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device-impl.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device-watcher.h"
)
set( DDS_CLIENT_SOURCE_FILES 
    "${CMAKE_CURRENT_LIST_DIR}/dds-guid.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dds-participant.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dds-topic.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dds-topic-reader.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/dds-device-watcher.cpp"
)
set( DDS_CLIENT_FILES ${DDS_CLIENT_HEADER_FILES} )
source_group( "DDS" FILES ${DDS_CLIENT_FILES} )

# NOTE: this requires that your "project(...)" statement match the executable
target_link_libraries( ${PROJECT_NAME} PRIVATE dds )
target_sources( ${PROJECT_NAME} PRIVATE ${DDS_CLIENT_FILES} )

include("${REPO_ROOT}/include/librealsense2/dds/topics/dds-topics.cmake")
# We don't include the following only because of librealsense: shared-init conflicts with log.cpp's
# INIT_EASYLOGGINGPP. Attempts to remove the latter cause problems with initialization orders and
# import of pyrealsense2 started crashing...
#include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")
include("${REPO_ROOT}/include/librealsense2/utilities/concurrency/CMakeLists.txt")

