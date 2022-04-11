# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
set( ELPP_INCLUDES ${REPO_ROOT}/third-party/easyloggingpp/src )
set( ELPP_SOURCES
    ${REPO_ROOT}/third-party/easyloggingpp/src/easylogging++.cc
    ${REPO_ROOT}/third-party/easyloggingpp/src/easylogging++.h
    ${CMAKE_CURRENT_LIST_DIR}/shared-init.cpp
)

source_group( "EasyLogging++" FILES ${ELPP_SOURCES} )
