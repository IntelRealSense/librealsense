# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.2)
cmake_policy(SET CMP0015 NEW)

project(tm)

set(SOURCE_FILES
    ../include/TrackingManager.h
    ../include/TrackingDevice.h
    ../include/TrackingCommon.h
    ../include/TrackingData.h

    Manager.h
    Manager.cpp
    Device.h
    Device.cpp
    Message.h
    UsbPlugListener.h
    UsbPlugListener.cpp
    CompleteTask.h
    Common.h
    Common.cpp
    
    infra/Log.h
    infra/Log.cpp
    infra/Event.h
    infra/Fence.h
    infra/EventHandler.h
    infra/Poller.h
    
    infra/Dispatcher.h
    infra/Dispatcher.cpp
    infra/Fsm.h
    infra/Fsm.cpp
    infra/Utils.h
    infra/Utils.cpp
    
    infra/Event_lin.cpp
    infra/Event_bsd.cpp
    infra/Event_win.cpp

    infra/Poller_lin.cpp
    infra/Poller_win.cpp
    infra/Poller_bsd.cpp
)

#Add versioning to DLL
IF(WIN32)
    set(SOURCE_FILES "${SOURCE_FILES};version.rc")
ENDIF(WIN32)

#Building Library
set(SDK_LIB_TYPE "STATIC")
add_definitions(-DBUILD_STATIC)

add_library(${PROJECT_NAME} ${SDK_LIB_TYPE} ${SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/../include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include> PRIVATE infra)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${OS_SPECIFIC_LIBS}
    usb
    fw
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBTM_VERSION_MAJOR}")
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Library)

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")

configure_file("${PROJECT_SOURCE_DIR}/Version.h.in"  "${CMAKE_CURRENT_BINARY_DIR}/Version.h")
configure_file("${PROJECT_SOURCE_DIR}/version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.rc")

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
