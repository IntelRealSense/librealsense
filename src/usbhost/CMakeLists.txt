# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE    
        "${CMAKE_CURRENT_LIST_DIR}/handle-usbhost.h"
        
        "${CMAKE_CURRENT_LIST_DIR}/usbhost.h"
        "${CMAKE_CURRENT_LIST_DIR}/usbhost.c"
        
        "${CMAKE_CURRENT_LIST_DIR}/endpoint-usbhost.h"
        "${CMAKE_CURRENT_LIST_DIR}/interface-usbhost.h"
        "${CMAKE_CURRENT_LIST_DIR}/interface-usbhost.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/device-usbhost.h"
        "${CMAKE_CURRENT_LIST_DIR}/device-usbhost.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/messenger-usbhost.h"
        "${CMAKE_CURRENT_LIST_DIR}/messenger-usbhost.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/enumerator-usbhost.cpp"
        
        "${CMAKE_CURRENT_LIST_DIR}/request-usbhost.h"
        "${CMAKE_CURRENT_LIST_DIR}/request-usbhost.cpp"
)
