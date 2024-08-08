# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/hid-types.h"
        "${CMAKE_CURRENT_LIST_DIR}/hid-device.h"
        "${CMAKE_CURRENT_LIST_DIR}/hid-device.cpp"
)

if(APPLE)

    find_library(CoreFoundationLib CoreFoundation)
    find_library(IOKitLib IOKit)

    target_sources(${LRS_TARGET}
        PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/../../third-party/hidapi/hidapi.h"
            "${CMAKE_CURRENT_LIST_DIR}/../../third-party/hidapi/hidapi.cpp"
    )

    target_include_directories(${LRS_TARGET} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../third-party/hidapi/)

    target_link_libraries(${LRS_TARGET} PRIVATE ${CoreFoundationLib} ${IOKitLib})

endif()
