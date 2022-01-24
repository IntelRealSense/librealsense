# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#target_sources(${LRS_TARGET}
#    PRIVATE
#        "${CMAKE_CURRENT_LIST_DIR}/rsusb-backend-linux.cpp"
#        "${CMAKE_CURRENT_LIST_DIR}/rsusb-backend-linux.h"
#
#)

target_link_libraries(${LRS_TARGET} PRIVATE dds)
if(BUILD_DDS_BACKEND)
    add_dependencies(${LRS_TARGET} dds)
endif()

