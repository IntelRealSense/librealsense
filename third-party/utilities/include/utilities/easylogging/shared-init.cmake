# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

if( BUILD_EASYLOGGINGPP  AND  BUILD_SHARED_LIBS )
    set( ELPP_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/shared-init.cpp
    )
    target_sources( ${PROJECT_NAME} PRIVATE ${ELPP_SOURCES} )
    source_group( "Common Files" FILES ${ELPP_SOURCES} )
endif()
