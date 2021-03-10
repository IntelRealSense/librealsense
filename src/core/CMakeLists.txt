# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/streaming.h"
        "${CMAKE_CURRENT_LIST_DIR}/debug.h"
        "${CMAKE_CURRENT_LIST_DIR}/advanced_mode.h"
        "${CMAKE_CURRENT_LIST_DIR}/roi.h"
        "${CMAKE_CURRENT_LIST_DIR}/motion.h"
        "${CMAKE_CURRENT_LIST_DIR}/video.h"
        "${CMAKE_CURRENT_LIST_DIR}/options.h"
        "${CMAKE_CURRENT_LIST_DIR}/info.h"
        "${CMAKE_CURRENT_LIST_DIR}/extension.h"
        "${CMAKE_CURRENT_LIST_DIR}/processing.h"
        "${CMAKE_CURRENT_LIST_DIR}/serialization.h"
)
