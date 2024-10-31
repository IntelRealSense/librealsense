# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/image-neon.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/neon-pointcloud.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/neon-align.cpp"
)
