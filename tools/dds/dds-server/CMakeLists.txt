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

include("${REPO_ROOT}/include/librealsense2/utilities/concurrency/CMakeLists.txt")
target_sources( ${PROJECT_NAME} PRIVATE ${CONCURRENCY_SOURCE_FILES} )
source_group( "Concurrency" FILES ${CONCURRENCY_SOURCE_FILES}  )

include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")
target_sources( ${PROJECT_NAME} PRIVATE ${ELPP_SOURCE_FILES} )
source_group( "EasyLogging++" FILES ${ELPP_SOURCE_FILES}  )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

