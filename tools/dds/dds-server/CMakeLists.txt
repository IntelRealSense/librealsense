# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
cmake_minimum_required( VERSION 3.1.0 )
project( rs-dds-server )

set(TOOL_FILES
    rs-dds-server.cpp
    lrs-device-watcher.h
    lrs-device-watcher.cpp
    lrs-device-controller.h
    lrs-device-controller.cpp
    )

add_executable(${PROJECT_NAME} ${TOOL_FILES} ${UTILITIES} ${ELPP_SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
target_include_directories( ${PROJECT_NAME} PRIVATE
    ${REPO_ROOT}/include
    ${REPO_ROOT}/third-party/tclap/include
)
target_link_libraries( ${PROJECT_NAME} PRIVATE realdds ${DEPENDENCIES} )
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Tools/dds)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include( "${REPO_ROOT}/third-party/utilities/include/utilities/easylogging/shared-init.cmake" )
