# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/ds6-device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-color.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-motion.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-active.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-private.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-factory.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-safety.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-device.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-color.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-motion.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-active.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-private.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-factory.h"
        "${CMAKE_CURRENT_LIST_DIR}/ds6-safety.h"
)
