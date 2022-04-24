# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/topics/dds-messages.h"
        )

include(${CMAKE_CURRENT_LIST_DIR}/topics/device-info/CMakeLists.txt)
