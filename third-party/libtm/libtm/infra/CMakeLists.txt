cmake_minimum_required(VERSION 2.8)
project(infra)

#Source Files
set(SOURCE_FILES
    Log.h
    Log.cpp
    Event.h
    Fence.h
    EventHandler.h
    Poller.h
    Poller_${OS}.cpp
    Dispatcher.h
    Dispatcher.cpp
    Fsm.h
    Fsm.cpp
    Utils.h
    Utils.cpp
    Semaphore.h
    Semaphore_${OS}.cpp
    Event_${OS}.cpp
)


#Building Library
set(SDK_LIB_TYPE "STATIC")
MESSAGE("Building project ${PROJECT_NAME} as ${SDK_LIB_TYPE} library")
add_library(${PROJECT_NAME} ${SDK_LIB_TYPE} ${SOURCE_FILES})

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Library)

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
