# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(rs-server VERSION 1.0.0 LANGUAGES CXX C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
# SET(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-pthread")

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_definitions(-DNEWLOCALE_NOT_USED=1 -DBSD=1 -DSOCKLEN_T=socklen_t -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -DALLOW_RTSP_SERVER_PORT_REUSE=1 -DNO_OPENSSL=1)

set(LIVE ${CMAKE_BINARY_DIR}/third-party/live)

file(GLOB RS_SERVER_SOURCES
    "*.h*"
    "*.c*"
	"../../src/ipDeviceCommon/*.h*"
	"../../src/ipDeviceCommon/*.c*"
    "${LIVE}/groupsock/*.c*"
    "${LIVE}/BasicUsageEnvironment/*.c*"
    "${LIVE}/liveMedia/*.c*"
    "${LIVE}/UsageEnvironment/*.c*"
)
# add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)

add_executable(${PROJECT_NAME} ${RS_SERVER_SOURCES})

include_directories(${PROJECT_NAME}
	../../common
	../../src/ipDeviceCommon
	../../include/librealsense2
	../../third-party/tclap/include
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

    # add_definitions(-DELPP_WINSOCK2)
        
    set(WINLIB Ws2_32.lib)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
  
target_link_libraries(${PROJECT_NAME} 
    PRIVATE ${WINLIB} realsense2 realsense2-compression Threads::Threads ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES}
)
  
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tools")
target_compile_definitions(${PROJECT_NAME} PUBLIC RESPONSE_BUFFER_SIZE=100000)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
