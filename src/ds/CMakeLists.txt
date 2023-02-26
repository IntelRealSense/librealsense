# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

include(${_rel_path}/ds/d400/CMakeLists.txt)

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/ds-device-common.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-motion-common.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-color-common.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-active-common.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-private.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-timestamp.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-options.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/advanced_mode/rs_advanced_mode.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/advanced_mode/presets.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/advanced_mode/advanced_mode.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-calib-parsers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-device-common.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds-motion-common.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds-color-common.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds-active-common.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds-private.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds-timestamp.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds-options.h"
        "${CMAKE_CURRENT_LIST_DIR}/advanced_mode/json_loader.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/advanced_mode/presets.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds-calib-parsers.h"
)
