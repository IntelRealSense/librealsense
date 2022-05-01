# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

include("${CMAKE_CURRENT_LIST_DIR}/device-info/device-info-topic-files.cmake")

set(DDS_TOPICS_FILES 
    ${DEVICE_INFO_TOPIC_FILES}
    "${CMAKE_CURRENT_LIST_DIR}/dds-messages.h"
)
