# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(realsense2-net VERSION 1.0.0 LANGUAGES CXX C)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_definitions(-DNEWLOCALE_NOT_USED=1 -DBSD=1 -DSOCKLEN_T=socklen_t -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -DALLOW_RTSP_SERVER_PORT_REUSE=1 -DNO_OPENSSL=1)

set(LIVE ${CMAKE_BINARY_DIR}/third-party/live)

file(GLOB REALSENSE_NET_CPP
    "*.h*"
    "*.c*"
    "realsense-net.def"
    "../ipDeviceCommon/*.h*"
    "../ipDeviceCommon/*.c*"
    "${LIVE}/groupsock/*.c*"
    "${LIVE}/BasicUsageEnvironment/*.c*"
    "${LIVE}/liveMedia/*.c*"
    "${LIVE}/UsageEnvironment/*.c*"
)

if (${BUILD_SHARED_LIBS} AND ${BUILD_EASYLOGGINGPP})
    list(APPEND REALSENSE_NET_CPP ../../third-party/easyloggingpp/src/easylogging++.cc)
endif()
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)

add_library(${PROJECT_NAME} ${REALSENSE_NET_CPP})

set(REALSENSE_NET_PUBLIC_HEADERS
    ../../include/librealsense2-net/rs_net.h
    ../../include/librealsense2-net/rs_net.hpp
)

set(CMAKECONFIG_NET_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

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

    add_definitions(-DELPP_WINSOCK2)
        
    set(WINLIB Ws2_32.lib)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE ${WINLIB} realsense2 realsense2-compression
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
