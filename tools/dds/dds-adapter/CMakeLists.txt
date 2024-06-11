# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required( VERSION 3.8 )
project( rs-dds-adapter )

set(TOOL_FILES
    rs-dds-adapter.cpp
    lrs-device-watcher.h
    lrs-device-watcher.cpp
    lrs-device-controller.h
    lrs-device-controller.cpp
    ../../../common/metadata-helper.cpp
    )

add_executable( ${PROJECT_NAME} ${TOOL_FILES} )
target_link_libraries( ${PROJECT_NAME} PRIVATE realdds realsense2 tclap )
set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER Tools/dds
    CXX_STANDARD 14
    )

install( TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

using_easyloggingpp( ${PROJECT_NAME} SHARED )

