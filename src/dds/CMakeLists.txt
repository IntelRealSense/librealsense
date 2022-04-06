# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/dds-device-watcher.h"
        "${CMAKE_CURRENT_LIST_DIR}/dds-device-watcher.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/msg/devicesPubSubTypes.h"
        "${CMAKE_CURRENT_LIST_DIR}/msg/devicesPubSubTypes.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/msg/devices.h"
        "${CMAKE_CURRENT_LIST_DIR}/msg/devices.cpp"
)

target_link_libraries(${LRS_TARGET} PRIVATE dds)

