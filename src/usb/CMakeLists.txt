# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/usb-endpoint.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb-interface.h" 
        "${CMAKE_CURRENT_LIST_DIR}/usb-messenger.h"
        
        "${CMAKE_CURRENT_LIST_DIR}/usb-types.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb-device.h"
        
        "${CMAKE_CURRENT_LIST_DIR}/usb-enumerator.h"
        "${CMAKE_CURRENT_LIST_DIR}/usb-request.h"      
)
