# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set(DEVICE_INFO_TOPIC_HEADER_FILES
   "${CMAKE_CURRENT_LIST_DIR}/deviceInfo.h"
   "${CMAKE_CURRENT_LIST_DIR}/device-info-msg.h"
   "${CMAKE_CURRENT_LIST_DIR}/deviceInfoPubSubTypes.h"
   "${CMAKE_CURRENT_LIST_DIR}/device-info-topic-files.cmake"
)
set( DEVICE_INFO_TOPIC_FILES ${DEVICE_INFO_TOPIC_HEADER_FILES} )
