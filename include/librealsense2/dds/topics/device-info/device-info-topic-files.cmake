# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set(DEVICE_INFO_TOPIC_HEADER_FILES
   "${CMAKE_CURRENT_LIST_DIR}/deviceInfo.h"
   "${CMAKE_CURRENT_LIST_DIR}/device-info-msg.h"
   "${CMAKE_CURRENT_LIST_DIR}/deviceInfoTypeObject.h"
   "${CMAKE_CURRENT_LIST_DIR}/deviceInfoPubSubTypes.h"
)
set(DEVICE_INFO_TOPIC_SOURCE_FILES
   "${CMAKE_CURRENT_LIST_DIR}/deviceInfo.cpp"
   "${CMAKE_CURRENT_LIST_DIR}/device-info-msg.cpp"
   "${CMAKE_CURRENT_LIST_DIR}/deviceInfoTypeObject.cpp"
   "${CMAKE_CURRENT_LIST_DIR}/deviceInfoPubSubTypes.cpp"
)
set( DEVICE_INFO_TOPIC_FILES ${DEVICE_INFO_TOPIC_HEADER_FILES} )
