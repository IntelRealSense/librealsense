# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

project( realdds )

add_library( ${PROJECT_NAME} STATIC )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Library )

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        "${REPO_ROOT}/include"  # we need the utilities
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries( ${PROJECT_NAME} PUBLIC dds )  # FastDDS


# Headers -----------------------------------------------------------------------------------
#
set( REALDDS_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include/realdds" )

set( REALDDS_COMMON_HEADER_FILES 
    "${REALDDS_INCLUDE_DIR}/dds-defines.h"
    "${REALDDS_INCLUDE_DIR}/dds-guid.h"
    "${REALDDS_INCLUDE_DIR}/dds-participant.h"
    "${REALDDS_INCLUDE_DIR}/dds-topic.h"
    "${REALDDS_INCLUDE_DIR}/dds-topic-reader.h"
    "${REALDDS_INCLUDE_DIR}/dds-utilities.h"
)
set( REALDDS_CLIENT_HEADER_FILES 
    "${REALDDS_INCLUDE_DIR}/dds-device.h"
    "${REALDDS_INCLUDE_DIR}/dds-device-impl.h"
    "${REALDDS_INCLUDE_DIR}/dds-device-watcher.h"
)
set( REALDDS_SERVER_HEADER_FILES 
    "${REALDDS_INCLUDE_DIR}/dds-device-broadcaster.h"
    "${REALDDS_INCLUDE_DIR}/dds-device-server.h"
)

set( REALDDS_CONTROL_TOPIC_HEADER_FILES
   "${REALDDS_INCLUDE_DIR}/topics/control/controlPubSubTypes.h"
   "${REALDDS_INCLUDE_DIR}/topics/control/control.h"
   "${REALDDS_INCLUDE_DIR}/topics/control/control-msg.h"
)
set( REALDDS_DEVICE_INFO_TOPIC_HEADER_FILES
   "${REALDDS_INCLUDE_DIR}/topics/device-info/deviceInfo.h"
   "${REALDDS_INCLUDE_DIR}/topics/device-info/device-info-msg.h"
   "${REALDDS_INCLUDE_DIR}/topics/device-info/deviceInfoPubSubTypes.h"
)
set( REALDDS_IMAGE_TOPIC_HEADER_FILES
   "${REALDDS_INCLUDE_DIR}/topics/image/imagePubSubTypes.h"
   "${REALDDS_INCLUDE_DIR}/topics/image/image.h"
   "${REALDDS_INCLUDE_DIR}/topics/image/image-msg.h"
)
set( REALDDS_NOTIFICATION_TOPIC_HEADER_FILES
   "${REALDDS_INCLUDE_DIR}/topics/notification/notificationPubSubTypes.h"
   "${REALDDS_INCLUDE_DIR}/topics/notification/notification.h"
   "${REALDDS_INCLUDE_DIR}/topics/notification/notification-msg.h"
)
set( REALDDS_TOPICS_HEADER_FILES 
    ${REALDDS_CONTROL_TOPIC_HEADER_FILES}
    ${REALDDS_DEVICE_INFO_TOPIC_HEADER_FILES}
    ${REALDDS_IMAGE_TOPIC_HEADER_FILES}
    ${REALDDS_NOTIFICATION_TOPIC_HEADER_FILES}
    "${REALDDS_INCLUDE_DIR}/topics/dds-topics.h"
)

set( REALDDS_HEADER_FILES
    ${REALDDS_COMMON_HEADER_FILES}
    ${REALDDS_CLIENT_HEADER_FILES}
    ${REALDDS_SERVER_HEADER_FILES}
    ${REALDDS_TOPICS_HEADER_FILES}
)
target_sources( ${PROJECT_NAME} PRIVATE ${REALDDS_HEADER_FILES} )

source_group( "Header Files/topics" FILES ${REALDDS_TOPICS_HEADER_FILES} )


# Sources -----------------------------------------------------------------------------------
#
set( REALDDS_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )

set( REALDDS_CONTROL_TOPIC_SOURCE_FILES
   "${REALDDS_SRC_DIR}/controlPubSubTypes.cpp"
   "${REALDDS_SRC_DIR}/controlTypeObject.h"
   "${REALDDS_SRC_DIR}/controlTypeObject.cpp"
   "${REALDDS_SRC_DIR}/control.cpp"
)
set( REALDDS_DEVICE_INFO_TOPIC_SOURCE_FILES
   "${REALDDS_SRC_DIR}/deviceInfo.cpp"
   "${REALDDS_SRC_DIR}/device-info-msg.cpp"
   "${REALDDS_SRC_DIR}/deviceInfoTypeObject.h"
   "${REALDDS_SRC_DIR}/deviceInfoTypeObject.cpp"
   "${REALDDS_SRC_DIR}/deviceInfoPubSubTypes.cpp"
)
set( REALDDS_IMAGE_TOPIC_SOURCE_FILES
   "${REALDDS_SRC_DIR}/imagePubSubTypes.cpp"
   "${REALDDS_SRC_DIR}/image.cpp"
)
set( REALDDS_NOTIFICATION_TOPIC_SOURCE_FILES
   "${REALDDS_SRC_DIR}/notificationPubSubTypes.cpp"
   "${REALDDS_SRC_DIR}/notificationTypeObject.h"
   "${REALDDS_SRC_DIR}/notificationTypeObject.cpp"
   "${REALDDS_SRC_DIR}/notification.cpp"
)
set( REALDDS_TOPICS_SOURCE_FILES
    ${REALDDS_CONTROL_TOPIC_SOURCE_FILES}
    ${REALDDS_DEVICE_INFO_TOPIC_SOURCE_FILES}
    ${REALDDS_IMAGE_TOPIC_SOURCE_FILES}
    ${REALDDS_NOTIFICATION_TOPIC_SOURCE_FILES}
)
source_group( "Source Files/topics" FILES ${REALDDS_TOPICS_SOURCE_FILES} )

SET( REALDDS_SOURCE_FILES
    "${REALDDS_SRC_DIR}/dds-guid.cpp"
    "${REALDDS_SRC_DIR}/dds-participant.cpp"
    "${REALDDS_SRC_DIR}/dds-topic.cpp"
    "${REALDDS_SRC_DIR}/dds-topic-reader.cpp"
    "${REALDDS_SRC_DIR}/dds-device.cpp"
    "${REALDDS_SRC_DIR}/dds-device-watcher.cpp"
    "${REALDDS_SRC_DIR}/dds-device-broadcaster.cpp"
    "${REALDDS_SRC_DIR}/dds-device-server.cpp"
    ${REALDDS_TOPICS_SOURCE_FILES}
)
target_sources( ${PROJECT_NAME} PRIVATE ${REALDDS_SOURCE_FILES} )


# Install -----------------------------------------------------------------------------------
#
install( TARGETS realdds EXPORT realsense2Targets)
