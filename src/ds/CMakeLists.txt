# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

include(${_rel_path}/ds/d400/CMakeLists.txt)
include(${_rel_path}/ds/d500/CMakeLists.txt)

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
        "${CMAKE_CURRENT_LIST_DIR}/ds-calib-common.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds-thermal-monitor.cpp"
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
        "${CMAKE_CURRENT_LIST_DIR}/ds-calib-common.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds-thermal-monitor.h"
        "${CMAKE_CURRENT_LIST_DIR}/features/amplitude-factor-feature.h"
        "${CMAKE_CURRENT_LIST_DIR}/features/amplitude-factor-feature.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/features/emitter-frequency-feature.h"
        "${CMAKE_CURRENT_LIST_DIR}/features/emitter-frequency-feature.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/features/auto-exposure-roi-feature.h"
        "${CMAKE_CURRENT_LIST_DIR}/features/auto-exposure-roi-feature.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/features/remove-ir-pattern-feature.h"
        "${CMAKE_CURRENT_LIST_DIR}/features/remove-ir-pattern-feature.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/features/auto-exposure-limit-feature.h"
        "${CMAKE_CURRENT_LIST_DIR}/features/auto-exposure-limit-feature.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/features/gain-limit-feature.h"
        "${CMAKE_CURRENT_LIST_DIR}/features/gain-limit-feature.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/features/gyro-sensitivity-feature.h"
        "${CMAKE_CURRENT_LIST_DIR}/features/gyro-sensitivity-feature.cpp"
)
