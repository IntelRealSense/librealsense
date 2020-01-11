# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/ivcam-private.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sr300.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sr300-fw-update-device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ivcam-private.h"
        "${CMAKE_CURRENT_LIST_DIR}/sr300.h"
        "${CMAKE_CURRENT_LIST_DIR}/sr300-fw-update-device.h"
)
