# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

include("${CMAKE_CURRENT_LIST_DIR}/device-info/device-info-topic-files.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/image/image-topic-files.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/notifications/notifications-topic-files.cmake")


set(DDS_TOPICS_FILES 
    ${DEVICE_INFO_TOPIC_FILES}
    ${IMAGE_TOPIC_FILES}
    ${NOTIFICATION_TOPIC_FILES}
    "${CMAKE_CURRENT_LIST_DIR}/dds-topics.h"
    )

source_group( "DDS/topics" FILES ${DDS_TOPICS_FILES} )

target_sources(${PROJECT_NAME}
    PRIVATE
        ${DDS_TOPICS_FILES}
    )

