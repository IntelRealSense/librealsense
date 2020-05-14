# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(realsense2-net VERSION 1.0.0 LANGUAGES CXX C)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_definitions(-DNEWLOCALE_NOT_USED=1 -DBSD=1 -DSOCKLEN_T=socklen_t -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -DALLOW_RTSP_SERVER_PORT_REUSE=1 -DNO_OPENSSL=1 -DRESPONSE_BUFFER_SIZE=100000)

#target_compile_definitions(${PROJECT_NAME} PUBLIC RESPONSE_BUFFER_SIZE=100000)

set(RS_CLIENT_NET_CPP
    ./client/RsMediaSession.cpp
    ./client/RsMediaSession.h
    ./client/RsNetDevice.cpp
    ./client/RsNetDevice.h
    ./client/RsRtpCallback.h
    ./client/RsRtpStream.h
    ./client/RsRtsp.h
    ./client/RsRtspClient.cpp
    ./client/RsRtspClient.h
    ./client/RsSink.cpp
    ./client/RsSink.h
)

set(RS_COMMON_NET_CPP
    ./common/RsCommon.h
    ./common/RsUsageEnvironment.cpp
    ./common/RsUsageEnvironment.h
)

set(RS_COMP_NET_CPP
    ./compression/CompressionFactory.cpp
    ./compression/CompressionFactory.h
    ./compression/ICompression.h
    ./compression/JpegCompression.cpp
    ./compression/JpegCompression.h
    ./compression/Lz4Compression.cpp
    ./compression/Lz4Compression.h
    ./compression/RvlCompression.cpp
    ./compression/RvlCompression.h
    ${LZ4_DIR}/lz4.h 
    ${LZ4_DIR}/lz4.c
)

set(RS_SERVER_NET_CPP
    ./server/RsDevice.cpp
    ./server/RsDevice.hh
    ./server/RsRTSPServer.cpp
    ./server/RsRTSPServer.hh
    ./server/RsSensor.cpp
    ./server/RsSensor.hh
    ./server/RsServer.cpp
    ./server/RsServer.h
    ./server/RsServerMediaSession.cpp
    ./server/RsServerMediaSession.h
    ./server/RsServerMediaSubsession.cpp
    ./server/RsServerMediaSubsession.h
    ./server/RsSimpleRTPSink.cpp
    ./server/RsSimpleRTPSink.h
    ./server/RsSource.cpp
    ./server/RsSource.hh
)

set(REALSENSE_NET_CPP 
    ${RS_CLIENT_NET_CPP} 
    ${RS_COMMON_NET_CPP} 
    ${RS_COMP_NET_CPP} 
    ${RS_SERVER_NET_CPP}
    rs_net.cpp
)

set(LIVE ${CMAKE_BINARY_DIR}/third-party/live)
file(GLOB LIVE555_CPP
    "${LIVE}/groupsock/*.c*"
    "${LIVE}/BasicUsageEnvironment/*.c*"
    "${LIVE}/liveMedia/*.c*"
    "${LIVE}/UsageEnvironment/*.c*"
)
list(REMOVE_ITEM REALSENSE_NET_CPP "${LIVE}/liveMedia/ADTSAudioStreamDiscreteFramer.cpp")

if (${BUILD_SHARED_LIBS} AND ${BUILD_EASYLOGGINGPP})
    list(APPEND REALSENSE_NET_CPP ../../third-party/easyloggingpp/src/easylogging++.cc)
endif()
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)

add_library(${PROJECT_NAME} ${REALSENSE_NET_CPP} ${LIVE555_CPP} ./realsense-net.def)

add_dependencies(${PROJECT_NAME} 
    libjpeg-turbo
)

set(REALSENSE_NET_PUBLIC_HEADERS
    ../../include/librealsense2-net/rs_net.h
    ../../include/librealsense2-net/rs_net.hpp
)

set(CMAKECONFIG_NET_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

include_directories(${PROJECT_NAME}
    ./
    ./common
    ./client
    ./server
    ./compression

    ../
    ../../common
    ../../third-party/easyloggingpp/src
    ../../third-party/tclap/include

    ${CMAKE_BINARY_DIR}/libjpeg-turbo/include

    ${LZ4_DIR}

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

    add_definitions(-DELPP_WINSOCK2)
        
    set(WINLIB Ws2_32.lib)
    set(JPGLIB "${CMAKE_BINARY_DIR}/libjpeg-turbo/lib/turbojpeg-static.lib")
else(WIN32)
    set(JPGLIB "${CMAKE_BINARY_DIR}/libjpeg-turbo/lib/libturbojpeg.a")
endif(WIN32)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE ${WINLIB} ${JPGLIB} realsense2 Threads::Threads
)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Library)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${REALSENSE_NET_PUBLIC_HEADERS}")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${REALSENSE_VERSION_STRING} SOVERSION "${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}")
# set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

WRITE_BASIC_CONFIG_VERSION_FILE("${CMAKE_CURRENT_BINARY_DIR}/realsense2-netConfigVersion.cmake"
    VERSION ${REALSENSE_VERSION_STRING} 
    COMPATIBILITY AnyNewerVersion
)

set(CMAKECONFIG_NET_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
configure_package_config_file(../../CMake/realsense2-netConfig.cmake.in realsense2-netConfig.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_NET_INSTALL_DIR}
    INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/bin
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)
configure_file(../../config/librealsense-net.pc.in ../../config/realsense2-net.pc @ONLY)

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2-netTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/librealsense2-net"
)

install(EXPORT realsense2-netTargets
    FILE realsense2-netTargets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKECONFIG_NET_INSTALL_DIR}
)

install(FILES "${CMAKE_BINARY_DIR}/src/ethernet/realsense2-netConfig.cmake"
    DESTINATION ${CMAKECONFIG_NET_INSTALL_DIR}
)

install(FILES "${CMAKE_BINARY_DIR}/src/ethernet/realsense2-netConfigVersion.cmake"
    DESTINATION ${CMAKECONFIG_NET_INSTALL_DIR}
)

# Set library pkgconfig file for facilitating 3rd party integration
install(FILES "${CMAKE_BINARY_DIR}/config/realsense2-net.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

install(CODE "execute_process(COMMAND ldconfig)")
