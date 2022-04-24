# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
project(rs-dds-server)

set(TOOL_FILES rs-dds-server.cpp
        dds-server.cpp
        dds-server.h)
        
set(TOPICS_FILES 
        ${REPO_ROOT}/include/librealsense2/dds/topics/device-info/deviceInfoPubSubTypes.h
        ${REPO_ROOT}/include/librealsense2/dds/topics/device-info/deviceInfo.h
        ${REPO_ROOT}/include/librealsense2/dds/topics/device-info/deviceInfoPubSubTypes.cpp
        ${REPO_ROOT}/include/librealsense2/dds/topics/device-info/deviceInfo.cpp
        ${REPO_ROOT}/include/librealsense2/dds/topics/device-info/device-info-msg.h
        ${REPO_ROOT}/include/librealsense2/dds/topics/dds-messages.h)

set(UTILITIES ${REPO_ROOT}/include/librealsense2/utilities/concurrency/dispatcher.cpp)


include(${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake)

add_executable(${PROJECT_NAME} ${TOOL_FILES} ${TOPICS_FILES} ${UTILITIES} ${ELPP_SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
include_directories(${REPO_ROOT}/third-party/tclap/include ${ELPP_INCLUDES} ${CMAKE_BINARY_DIR} )
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES} dds)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Tools/dds)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
