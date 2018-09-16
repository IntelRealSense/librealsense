cmake_minimum_required(VERSION 2.8)

set(LIBTM_HEADER_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
set(LIBTM_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

set(HEADER_FILES_LIBTM
    ${CMAKE_CURRENT_LIST_DIR}/include/TrackingManager.h
    ${CMAKE_CURRENT_LIST_DIR}/include/TrackingDevice.h
    ${CMAKE_CURRENT_LIST_DIR}/include/TrackingCommon.h
    ${CMAKE_CURRENT_LIST_DIR}/include/TrackingData.h
)

set(SOURCE_FILES_LIBTM
    ${CMAKE_CURRENT_LIST_DIR}/src/Manager.h
    ${CMAKE_CURRENT_LIST_DIR}/src/Manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Device.h
    ${CMAKE_CURRENT_LIST_DIR}/src/Device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Message.h
	${CMAKE_CURRENT_LIST_DIR}/src/UsbPlugListener.h
	${CMAKE_CURRENT_LIST_DIR}/src/UsbPlugListener.cpp
	${CMAKE_CURRENT_LIST_DIR}/src/CompleteTask.h
	${CMAKE_CURRENT_LIST_DIR}/src/Common.h
	${CMAKE_CURRENT_LIST_DIR}/src/Common.cpp
)
