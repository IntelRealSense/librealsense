# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/win-helpers.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win-uvc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win-usb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win-hid.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/win-backend.cpp"
)
