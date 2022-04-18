# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/dds-device-watcher.h"
        "${CMAKE_CURRENT_LIST_DIR}/dds-device-watcher.cpp"
)

target_link_libraries(${LRS_TARGET} PRIVATE dds)

