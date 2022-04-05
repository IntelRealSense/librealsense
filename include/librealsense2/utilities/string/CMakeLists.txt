# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

SET(STRING_TARGETS "${CMAKE_CURRENT_LIST_DIR}/split.h"
        "${CMAKE_CURRENT_LIST_DIR}/string-utilities.h"
        "${CMAKE_CURRENT_LIST_DIR}/trim-newlines.h")
        
if(WIN32)
    SET(STRING_TARGETS ${STRING_TARGETS} "${CMAKE_CURRENT_LIST_DIR}/windows.h")
endif()      


target_sources(${LRS_TARGET}
    PRIVATE
    ${STRING_TARGETS})

