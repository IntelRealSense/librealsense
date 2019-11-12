# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/handle-winusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/endpoint-winusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/interface-winusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/interface-winusb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/device-winusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/device-winusb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/enumerator-winusb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/messenger-winusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/messenger-winusb.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/request-winusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/request-winusb.cpp"
)
