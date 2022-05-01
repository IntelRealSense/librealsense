# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

include("${CMAKE_CURRENT_LIST_DIR}/topics/dds-messages.cmake")

target_sources(${LRS_TARGET}
    PRIVATE
        ${DDS_TOPICS_FILES}
        )

