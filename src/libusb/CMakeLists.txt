# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/context-libusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/context-libusb.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/handle-libusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/endpoint-libusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/interface-libusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/interface-libusb.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/device-libusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/device-libusb.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/messenger-libusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/messenger-libusb.cpp"

        "${CMAKE_CURRENT_LIST_DIR}/request-libusb.h"
        "${CMAKE_CURRENT_LIST_DIR}/request-libusb.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/enumerator-libusb.cpp"
)
