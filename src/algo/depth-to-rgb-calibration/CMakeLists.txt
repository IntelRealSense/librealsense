# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/calibration-types.h"
        "${CMAKE_CURRENT_LIST_DIR}/calibration.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/calibration.h"
        "${CMAKE_CURRENT_LIST_DIR}/coeffs.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/coeffs.h"
        "${CMAKE_CURRENT_LIST_DIR}/debug.h"
        "${CMAKE_CURRENT_LIST_DIR}/frame-data.h"
        "${CMAKE_CURRENT_LIST_DIR}/optimizer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/optimizer.h"
        "${CMAKE_CURRENT_LIST_DIR}/rotation-in-angles.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/valid-scene.cpp"
)
