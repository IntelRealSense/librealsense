# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/sql.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/recorder.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/sql.h"
        "${CMAKE_CURRENT_LIST_DIR}/recorder.h"
)
