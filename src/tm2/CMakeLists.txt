# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/tm-boot.h"
        "${CMAKE_CURRENT_LIST_DIR}/tm-device.h"
        "${CMAKE_CURRENT_LIST_DIR}/tm-info.h"
        "${CMAKE_CURRENT_LIST_DIR}/tm-device.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/tm-info.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/t265-messages.h"
)

target_link_libraries(${LRS_TARGET} PRIVATE fw) # for T265 FW
target_include_directories(${LRS_TARGET} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
set_target_properties (${LRS_TARGET} PROPERTIES FOLDER Resources)
