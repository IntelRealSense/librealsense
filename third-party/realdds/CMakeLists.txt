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
include("${REPO_ROOT}/include/librealsense2/dds/dds-client.cmake")


set( REALDDS_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}" )


set( CONTROL_TOPIC_SOURCE_FILES
   "${REALDDS_SRC_DIR}/controlPubSubTypes.cpp"
   "${REALDDS_SRC_DIR}/controlTypeObject.h"
   "${REALDDS_SRC_DIR}/controlTypeObject.cpp"
   "${REALDDS_SRC_DIR}/control.cpp"
)
set( DEVICE_INFO_TOPIC_SOURCE_FILES
   "${REALDDS_SRC_DIR}/deviceInfo.cpp"
   "${REALDDS_SRC_DIR}/device-info-msg.cpp"
   "${REALDDS_SRC_DIR}/deviceInfoTypeObject.h"
   "${REALDDS_SRC_DIR}/deviceInfoTypeObject.cpp"
   "${REALDDS_SRC_DIR}/deviceInfoPubSubTypes.cpp"
)
set( IMAGE_TOPIC_SOURCE_FILES
   "${REALDDS_SRC_DIR}/imagePubSubTypes.cpp"
   "${REALDDS_SRC_DIR}/image.cpp"
)
set( NOTIFICATION_TOPIC_SOURCE_FILES
   "${REALDDS_SRC_DIR}/notificationPubSubTypes.cpp"
   "${REALDDS_SRC_DIR}/notificationTypeObject.h"
   "${REALDDS_SRC_DIR}/notificationTypeObject.cpp"
   "${REALDDS_SRC_DIR}/notification.cpp"
)
set( TOPICS_SOURCE_FILES
    ${CONTROL_TOPIC_SOURCE_FILES}
    ${DEVICE_INFO_TOPIC_SOURCE_FILES}
    ${IMAGE_TOPIC_SOURCE_FILES}
    ${NOTIFICATION_TOPIC_SOURCE_FILES}
)
source_group( "Source Files/topics" FILES ${TOPICS_SOURCE_FILES} )

SET( REALDDS_SOURCE_FILES
    "${REALDDS_SRC_DIR}/dds-guid.cpp"
    "${REALDDS_SRC_DIR}/dds-participant.cpp"
    "${REALDDS_SRC_DIR}/dds-topic.cpp"
    "${REALDDS_SRC_DIR}/dds-topic-reader.cpp"
    "${REALDDS_SRC_DIR}/dds-device.cpp"
    "${REALDDS_SRC_DIR}/dds-device-watcher.cpp"
    "${REALDDS_SRC_DIR}/dds-device-broadcaster.cpp"
    "${REALDDS_SRC_DIR}/dds-device-server.cpp"
    ${TOPICS_SOURCE_FILES}
)
target_sources( ${PROJECT_NAME} PRIVATE ${REALDDS_SOURCE_FILES} )

SET( REALDDS_FILES
    ${REALDDS_SOURCE_FILES}
    ${TOPICS_SOURCE_FILES}
)


install( TARGETS realdds EXPORT realsense2Targets)
