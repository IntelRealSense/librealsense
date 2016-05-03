cmake_minimum_required(VERSION 3.1)

project(Realsense)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(REALSENSE_CPP 
    src/context.cpp
    src/device.cpp
    src/f200-private.cpp
    src/f200.cpp
    src/image.cpp
    src/log.cpp
    src/r200-private.cpp
    src/r200.cpp
    src/rs.cpp
    src/stream.cpp
    src/sync.cpp
    src/types.cpp
    src/uvc-libuvc.cpp
    src/uvc-v4l2.cpp
    src/uvc-wmf.cpp
    src/uvc.cpp
    src/verify.c
)

set(REALSENSE_HPP 
    src/context.h
    src/device.h
    src/f200-private.h
    src/f200.h
    src/image.h
    src/r200-private.h
    src/r200.h
    src/stream.h
    src/sync.h
    src/types.h
    src/uvc.h
)

if(WIN32)
    set(BACKEND RS_USE_WMF_BACKEND)
    set(REALSENSE_DEF CMake/realsense.def)
elseif(APPLE)
    set(BACKEND RS_USE_LIBUVC_BACKEND)
else()
    set(BACKEND RS_USE_V4L2_BACKEND)
endif()
add_definitions(-D${BACKEND} -DUNICODE)

if(UNIX)
    list(APPEND REALSENSE_CPP
        src/libuvc/ctrl.c
        src/libuvc/dev.c
        src/libuvc/diag.c
        src/libuvc/frame.c
        src/libuvc/init.c
        src/libuvc/stream.c
    )
    list(APPEND REALSENSE_HPP
        src/libuvc/libuvc_config.h
        src/libuvc/libuvc.h
        src/libuvc/libuvc_internal.h
        src/libuvc/utlist.h
    )
    
    find_package(LibUSB REQUIRED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pedantic -mssse3 -Ofast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-multichar")
endif()

add_library(realsense SHARED ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_DEF})
target_link_libraries(realsense ${LibUSB_LIBRARIES})
target_include_directories(realsense PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${LibUSB_INCLUDE_DIRS})

install( TARGETS realsense
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/librealsense DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

option(BUILD_EXAMPLES "Build realsense examples." OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

