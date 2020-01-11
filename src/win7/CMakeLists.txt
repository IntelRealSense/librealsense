# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/rsusb-backend-windows.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/rsusb-backend-windows.h"

)

include(libusb_config)
target_link_libraries(${LRS_TARGET} PRIVATE usb)
if(USE_EXTERNAL_USB)
    add_dependencies(${LRS_TARGET} libusb)
endif()

