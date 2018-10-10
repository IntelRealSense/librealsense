cmake_minimum_required(VERSION 2.8)

set(INFRA_HEADER_DIR ${CMAKE_CURRENT_LIST_DIR}/include)
set(INFRA_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

#Header Files
set(HEADER_FILES_INFRA
    ${CMAKE_CURRENT_LIST_DIR}/include/Utils.h
	${CMAKE_CURRENT_LIST_DIR}/include/Log.h
	${CMAKE_CURRENT_LIST_DIR}/include/Dispatcher.h
	${CMAKE_CURRENT_LIST_DIR}/include/Fsm.h
	${CMAKE_CURRENT_LIST_DIR}/include/Event.h
    ${CMAKE_CURRENT_LIST_DIR}/include/Fence.h
    ${CMAKE_CURRENT_LIST_DIR}/include/EventHandler.h
    ${CMAKE_CURRENT_LIST_DIR}/include/Semaphore.h
    ${CMAKE_CURRENT_LIST_DIR}/include/Poller.h
)

#Source Files
set(SOURCE_FILES_INFRA
    ${CMAKE_CURRENT_LIST_DIR}/src/Log.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Poller_${OS}.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Dispatcher.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Fsm.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Semaphore_${OS}.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Event_${OS}.cpp
)