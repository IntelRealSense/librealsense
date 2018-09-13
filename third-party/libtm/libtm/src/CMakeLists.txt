cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0015 NEW)

project(tm)

include_directories(${LIBTM_INCLUDE_DIR})
include_directories(${LIBTM_INFRA_DIR})

#Source Files
set(SOURCE_FILES
    ${LIBTM_INCLUDE_DIR}/TrackingManager.h
    ${LIBTM_INCLUDE_DIR}/TrackingDevice.h
    ${LIBTM_INCLUDE_DIR}/TrackingCommon.h
    ${LIBTM_INCLUDE_DIR}/TrackingData.h
    ${LIBTM_INCLUDE_DIR}/TrackingSerializer.h
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

	RcSerializer/Recorder.h
	RcSerializer/Recorder.cpp
	RcSerializer/Packet.h
	RcSerializer/Packet.cpp
	RcSerializer/latency_queue.h
	RcSerializer/rc_packet.h
	RcSerializer/concurrency.h
	RcSerializer/Player.h
	RcSerializer/Player.cpp
)

#Add versioning to DLL
IF(WIN32)
SET(SOURCE_FILES "${SOURCE_FILES};version.rc")
ENDIF(WIN32)

#Building Library
MESSAGE("Building project ${PROJECT_NAME} as ${LIB_TYPE} library")
add_library(${PROJECT_NAME} ${LIB_TYPE} ${SOURCE_FILES})

#LINK_LIBRARIES
target_link_libraries(${PROJECT_NAME}
infra
${OS_SPECIFIC_LIBS}
${CMAKE_SOURCE_DIR}/third-party/win/libusb/bin/x64/libusb-1.0.lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBVERSION}" SOVERSION "${LIBTM_VERSION_MAJOR}")
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Library)

#install(TARGETS ${PROJECT_NAME} DESTINATION lib)

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

