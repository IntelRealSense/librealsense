# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.
target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/fw-log-data.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/fw-log-data.h"
        "${CMAKE_CURRENT_LIST_DIR}/fw-logs-formating-options.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/fw-logs-formating-options.h"
        "${CMAKE_CURRENT_LIST_DIR}/fw-logs-parser.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/fw-logs-parser.h"
        "${CMAKE_CURRENT_LIST_DIR}/fw-logs-xml-helper.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/fw-logs-xml-helper.h"
        "${CMAKE_CURRENT_LIST_DIR}/fw-string-formatter.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/fw-string-formatter.h"
)
