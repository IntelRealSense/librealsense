# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.

set(COMMON_SRC
        "${CMAKE_CURRENT_LIST_DIR}/rendering.h"
        "${CMAKE_CURRENT_LIST_DIR}/model-views.h"
        "${CMAKE_CURRENT_LIST_DIR}/model-views.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/notifications.h"
        "${CMAKE_CURRENT_LIST_DIR}/notifications.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/viewer.h"
        "${CMAKE_CURRENT_LIST_DIR}/viewer.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ux-window.h"
        "${CMAKE_CURRENT_LIST_DIR}/ux-window.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ux-alignment.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/ux-alignment.h"
        "${CMAKE_CURRENT_LIST_DIR}/opengl3.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/opengl3.h"
        "${CMAKE_CURRENT_LIST_DIR}/rs-config.h"
        "${CMAKE_CURRENT_LIST_DIR}/rs-config.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/os.h"
        "${CMAKE_CURRENT_LIST_DIR}/os.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/fw-update-helper.h"
        "${CMAKE_CURRENT_LIST_DIR}/fw-update-helper.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/metadata-helper.h"
        "${CMAKE_CURRENT_LIST_DIR}/metadata-helper.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/on-chip-calib.h"
        "${CMAKE_CURRENT_LIST_DIR}/on-chip-calib.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/decompress-huffman.h"
)
