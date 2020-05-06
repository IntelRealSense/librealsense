# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/cuda-align.h"
        "${CMAKE_CURRENT_LIST_DIR}/cuda-align.cu"
        "${CMAKE_CURRENT_LIST_DIR}/cuda-align.cuh"
        "${CMAKE_CURRENT_LIST_DIR}/cuda-pointcloud.h"
        "${CMAKE_CURRENT_LIST_DIR}/cuda-pointcloud.cpp"

)
