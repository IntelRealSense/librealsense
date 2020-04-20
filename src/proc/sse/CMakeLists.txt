# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/sse-align.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sse-align.h"
        "${CMAKE_CURRENT_LIST_DIR}/sse-pointcloud.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sse-pointcloud.h"
)
