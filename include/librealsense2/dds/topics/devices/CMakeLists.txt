# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/devicesMsgPubSubTypes.h"
        "${CMAKE_CURRENT_LIST_DIR}/devicesMsgPubSubTypes.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/devicesMsg.h"
        "${CMAKE_CURRENT_LIST_DIR}/devicesMsg.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/dds-devices-msg.h"
        )
