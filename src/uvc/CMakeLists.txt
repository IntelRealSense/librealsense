# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/uvc-types.h"
        "${CMAKE_CURRENT_LIST_DIR}/uvc-device.h"
        "${CMAKE_CURRENT_LIST_DIR}/uvc-device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/uvc-parser.h"
        "${CMAKE_CURRENT_LIST_DIR}/uvc-parser.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/uvc-streamer.h"
        "${CMAKE_CURRENT_LIST_DIR}/uvc-streamer.cpp"
)
