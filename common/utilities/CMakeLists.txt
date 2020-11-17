# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.

target_sources(${LRS_TARGET}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/time/common.h"
        "${CMAKE_CURRENT_LIST_DIR}/time/stopwatch.h"
        "${CMAKE_CURRENT_LIST_DIR}/time/timer.h"
        "${CMAKE_CURRENT_LIST_DIR}/time/periodic_timer.h"
        "${CMAKE_CURRENT_LIST_DIR}/time/work_week.h"
        "${CMAKE_CURRENT_LIST_DIR}/time/work_week.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/time/l500/get-mfr-ww.h"
        "${CMAKE_CURRENT_LIST_DIR}/time/l500/get-mfr-ww.cpp"
)
