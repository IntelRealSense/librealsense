# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set(NOTIFICATION_TOPIC_HEADER_FILES
   "${CMAKE_CURRENT_LIST_DIR}/notificationPubSubTypes.h"
   "${CMAKE_CURRENT_LIST_DIR}/notification.h"
   "${CMAKE_CURRENT_LIST_DIR}/notification-msg.h"
   "${CMAKE_CURRENT_LIST_DIR}/notification-topic-files.cmake"
)
set( NOTIFICATION_TOPIC_FILES ${NOTIFICATION_TOPIC_HEADER_FILES} )
