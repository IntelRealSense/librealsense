# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

include(${CMAKE_CURRENT_LIST_DIR}/topics/device-info/device_info.cmake)

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/topics/dds-messages.h"
        ${DEVICE_INFO_TOPIC_FILES}
        )

