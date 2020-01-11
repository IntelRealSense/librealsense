# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/mf-uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/mf-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/mf-backend.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/mf-uvc.h"
        "${CMAKE_CURRENT_LIST_DIR}/mf-hid.h"
        "${CMAKE_CURRENT_LIST_DIR}/mf-backend.h"
)
