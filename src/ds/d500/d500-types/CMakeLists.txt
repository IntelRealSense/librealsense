# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 RealSense, Inc. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/application-config.h"
        "${CMAKE_CURRENT_LIST_DIR}/calibration-config.h"
        "${CMAKE_CURRENT_LIST_DIR}/safety-interface-config.h"
        "${CMAKE_CURRENT_LIST_DIR}/safety-preset.h"
        "${CMAKE_CURRENT_LIST_DIR}/common.h"
)
