# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/l500.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/l500-private.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/l500.h"
        "${CMAKE_CURRENT_LIST_DIR}/l500-private.h"
)
