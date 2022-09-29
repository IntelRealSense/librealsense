# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

project( realdds )

add_library( ${PROJECT_NAME} STATIC )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Library )

# Add outselves to the dependencies of other librealsense
# TODO: requires policy CMP0079, requiring CMake 3.13, requiring Ubuntu 2020!
#target_link_libraries( ${LRS_TARGET} PRIVATE realdds )

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "${REPO_ROOT}/include"
    )


include("${REPO_ROOT}/include/librealsense2/dds/dds-server.cmake")
source_group( "DDS/server" FILES ${DDS_SERVER_SOURCE_FILES} )
target_sources( ${PROJECT_NAME} PRIVATE ${DDS_SERVER_SOURCE_FILES} )

#include("${REPO_ROOT}/include/librealsense2/dds/topics/dds-topics.cmake")
source_group( "DDS/topics" FILES ${DDS_TOPICS_SOURCE_FILES} )
target_sources( ${PROJECT_NAME} PRIVATE ${DDS_TOPICS_SOURCE_FILES} )

#include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")


install( TARGETS realdds EXPORT realsense2Targets)


include("${REPO_ROOT}/include/librealsense2/dds/dds-client.cmake")
source_group( "DDS/client" FILES ${DDS_CLIENT_SOURCE_FILES} )
target_sources( ${PROJECT_NAME} PRIVATE ${DDS_CLIENT_SOURCE_FILES} )

#include("${REPO_ROOT}/include/librealsense2/dds/topics/dds-topics.cmake")

#include("${REPO_ROOT}/include/librealsense2/utilities/concurrency/CMakeLists.txt")

