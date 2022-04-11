# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

set( ELPP_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/../../../../third-party/easyloggingpp/src )
set( ELPP_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../../../third-party/easyloggingpp/src/easylogging++.cc
    ${CMAKE_CURRENT_LIST_DIR}/../../../../third-party/easyloggingpp/src/easylogging++.h
    ${CMAKE_CURRENT_LIST_DIR}/shared-init.cpp
)

source_group( "EasyLogging++" FILES ${ELPP_SOURCES} )