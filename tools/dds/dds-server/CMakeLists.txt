# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
project(rs-dds-server)

set(TOOL_FILES rs-dds-server.cpp
        dds-server.cpp
        dds-server.h)
        
include("${REPO_ROOT}/include/librealsense2/dds/topics/dds-messages.cmake")
include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")

set(UTILITIES "${REPO_ROOT}/include/librealsense2/utilities/concurrency/dispatcher.cpp")

add_executable(${PROJECT_NAME} ${TOOL_FILES} ${DDS_TOPICS_FILES} ${UTILITIES} ${ELPP_SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
include_directories(${REPO_ROOT}/third-party/tclap/include ${ELPP_INCLUDES} ${CMAKE_BINARY_DIR} )
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES} dds)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Tools/dds)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
