# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set( CONCURRENCY_HEADER_FILES
    "${CMAKE_CURRENT_LIST_DIR}/concurrency.h"
)
set( CONCURRENCY_SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/dispatcher.cpp"
)
set( CONCURRENCY_FILES ${CONCURRENCY_HEADER_FILES} )
target_sources( ${PROJECT_NAME} PRIVATE ${CONCURRENCY_FILES} )
source_group( "Concurrency" FILES ${CONCURRENCY_FILES}  )

