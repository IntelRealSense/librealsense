# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.9.0)

project(realsense2-ethernet VERSION 1.0.0 LANGUAGES CXX C)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(DEPENDENCIES ${DEPENDENCIES} realsense2)

set(CMAKE_CXX_FLAGS "-DNEWLOCALE_NOT_USED=1 -DBSD=1 -DSOCKLEN_T=socklen_t -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -DALLOW_RTSP_SERVER_PORT_REUSE=1 -DNO_OPENSSL=1")

set(LIVE ${CMAKE_BINARY_DIR}/third-party/live)

file(GLOB IPDEV_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS 
  "*.h*"
  "*.c*"
  "../ipDeviceCommon/*.h*"
  "../ipDeviceCommon/*.c*"
  "${LIVE}/*.c*"
  "${LIVE}/*.h*"
  "${LIVE}/groupsock/*.c*"
  "${LIVE}/BasicUsageEnvironment/*.c*"
  "${LIVE}/liveMedia/*.c*"
  "${LIVE}/UsageEnvironment/*.c*"
)
add_library(${PROJECT_NAME} SHARED ${IPDEV_SOURCES})

include_directories(${PROJECT_NAME}
  ../../common
  ../ipDeviceCommon
  ../../third-party/easyloggingpp/src
  ${LIVE}/groupsock/include 
  ${LIVE}/liveMedia/include  
  ${LIVE}/UsageEnvironment/include 
  ${LIVE}/BasicUsageEnvironment/include 
)

if(WIN32)
        #
        # Windows doesn't support POSIX ssize_t type
        # by default so I use the following defines:
        #
        # #ifdef _WIN64
        # #  define ssize_t __int64
        # #else
        # #  define ssize_t long
        # #endif
        #
        if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
                # 64-bit environment...
                message(STATUS "Target is 64 bits")
                add_definitions(-Dssize_t=__int64)
        else("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
                # 32-bit environment...
                message(STATUS "Target is 32 bits")
                add_definitions(-Dssize_t=long)
        endif("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

target_link_libraries(${PROJECT_NAME} 
        PRIVATE ${DEPENDENCIES}
        realsense2-compression
)

if(IPDEV_STATS)
    add_definitions(-DSTATISTICS)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Library")

if(WIN32)
        install(TARGETS ${PROJECT_NAME}
                RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
                PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
else()
        install(TARGETS ${PROJECT_NAME}
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
endif()
