# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/d400-options.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-private.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-motion.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-nonmonochrome.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-color.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-active.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-factory.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-fw-update-device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-auto-calibration.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/d400-info.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-device.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-options.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-private.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-active.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-motion.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-nonmonochrome.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-color.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-fw-update-device.h"
        "${CMAKE_CURRENT_LIST_DIR}/d400-auto-calibration.h"
)
