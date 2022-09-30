# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

include("${CMAKE_CURRENT_LIST_DIR}/device-info/device-info-topic-files.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/image/image-topic-files.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/notification/notification-topic-files.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/control/control-topic-files.cmake")


set(DDS_TOPICS_HEADER_FILES 
    ${DEVICE_INFO_TOPIC_HEADER_FILES}
    ${IMAGE_TOPIC_HEADER_FILES}
    ${NOTIFICATION_TOPIC_HEADER_FILES}
    ${CONTROL_TOPIC_HEADER_FILES}
    "${CMAKE_CURRENT_LIST_DIR}/dds-topics.h"
    "${CMAKE_CURRENT_LIST_DIR}/dds-topics.cmake"
)
set( DDS_TOPICS_FILES ${DDS_TOPICS_HEADER_FILES} )
source_group( "DDS/topics" FILES ${DDS_TOPICS_FILES} )

target_sources(${PROJECT_NAME}
    PRIVATE
        ${DDS_TOPICS_FILES}
    )

