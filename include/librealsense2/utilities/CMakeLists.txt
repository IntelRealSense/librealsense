# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

include(${CMAKE_CURRENT_LIST_DIR}/easylogging/CMakeLists.txt)
target_sources( ${PROJECT_NAME} PRIVATE ${ELPP_SOURCE_FILES} )
source_group( "EasyLogging++" FILES ${ELPP_SOURCE_FILES}  )
include(${CMAKE_CURRENT_LIST_DIR}/concurrency/CMakeLists.txt)
target_sources( ${PROJECT_NAME} PRIVATE ${CONCURRENCY_SOURCE_FILES} )
source_group( "Concurrency" FILES ${CONCURRENCY_SOURCE_FILES}  )
include(${CMAKE_CURRENT_LIST_DIR}/time/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/number/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/os/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/string/CMakeLists.txt)

target_sources( ${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/json.h"
        "${CMAKE_CURRENT_LIST_DIR}/shared-ptr-singleton.h"
    )

