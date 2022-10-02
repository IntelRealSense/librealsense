# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
if(${BUILD_EASYLOGGINGPP})

set( ELPP_INCLUDES ${REPO_ROOT}/third-party/easyloggingpp/src )
set( ELPP_HEADER_FILES
    ${REPO_ROOT}/third-party/easyloggingpp/src/easylogging++.h
    ${CMAKE_CURRENT_LIST_DIR}/easyloggingpp.cmake
    )
set( ELPP_SOURCE_FILES
    ${REPO_ROOT}/third-party/easyloggingpp/src/easylogging++.cc
    ${CMAKE_CURRENT_LIST_DIR}/shared-init.cpp
    )
set( ELPP_FILES ${ELPP_HEADER_FILES} )

source_group( "EasyLogging++" FILES ${ELPP_FILES} )

target_sources( ${PROJECT_NAME}
    PRIVATE
        ${ELPP_FILES}
    )
target_include_directories( ${PROJECT_NAME}
    PRIVATE
        ${ELPP_INCLUDES}
    )

endif()  # BUILD_EASYLOGGINGPP

