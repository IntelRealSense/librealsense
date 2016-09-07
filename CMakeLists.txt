cmake_minimum_required(VERSION 2.8.3)

project(Realsense)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

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
    
    find_package(PkgConfig REQUIRED)
    pkg_search_module(LIBUSB1 REQUIRED libusb-1.0)
    if(LIBUSB1_FOUND)
      include_directories(SYSTEM ${LIBUSB1_INCLUDE_DIRS})
    else()
        message( FATAL_ERROR "Failed to find libusb-1.0" )
    endif(LIBUSB1_FOUND)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pedantic -mssse3 -Ofast")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-multichar")
endif()

option(BUILD_SHARED_LIBS "Build shared library" ON)
if(BUILD_SHARED_LIBS)
    add_library(realsense SHARED ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_DEF})
    target_link_libraries(realsense ${LIBUSB_LIBRARIES})
else()
    add_library(realsense STATIC ${REALSENSE_CPP} ${REALSENSE_HPP})
endif()

target_include_directories(realsense PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${LIBUSB_INCLUDE_DIRS})

install( TARGETS realsense
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/librealsense DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

option(BUILD_EXAMPLES "Build realsense examples." ON)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

