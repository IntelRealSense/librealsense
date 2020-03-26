# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(rs-server VERSION 1.0.0 LANGUAGES CXX C)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(DEPENDENCIES ${DEPENDENCIES} realsense2 Threads::Threads)

set(CMAKE_CXX_FLAGS "-DNEWLOCALE_NOT_USED=1 -DBSD=1 -DSOCKLEN_T=socklen_t -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -DALLOW_RTSP_SERVER_PORT_REUSE=1 -DNO_OPENSSL=1 -latomic")

SET(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-pthread")

set(LIVE ${CMAKE_BINARY_DIR}/third-party/live)

if(WIN32)
  message(SEND_ERROR "rs-server supports Linux only")
else()
  file(GLOB RS_SERVER_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS
    "*.c*"
    "../../src/ipDeviceCommon/*.c*"
    "${LIVE}/*.c*"
    "${LIVE}/groupsock/*.c*"
    "${LIVE}/BasicUsageEnvironment/*.c*"
    "${LIVE}/liveMedia/*.c*"
    "${LIVE}/UsageEnvironment/*.c*"
  )
  add_executable(${PROJECT_NAME} ${RS_SERVER_SOURCES})
  add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)

  include_directories(${PROJECT_NAME}
    ../../common
    ../../src/ipDeviceCommon
    ../../include/librealsense2
    ../../third-party/easyloggingpp/src
    ${LIVE}/groupsock/include 
    ${LIVE}/liveMedia/include  
    ${LIVE}/UsageEnvironment/include 
    ${LIVE}/BasicUsageEnvironment/include     
  )

  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

  set(DEPENDENCIES ${DEPENDENCIES} realsense2 Threads::Threads realsense2-compression ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES})
  
  target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES})
  
  set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Tools")

  target_compile_definitions(${PROJECT_NAME} PUBLIC RESPONSE_BUFFER_SIZE=100000)

  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()


