# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.9.0)

project(realsense2-compression VERSION 1.0.0 LANGUAGES CXX C)
# Save the command line compile commands in the build output

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(DEPENDENCIES ${DEPENDENCIES} realsense2)

file(GLOB COMPRESSION_SOURCES 
 "*.h"
 "*.cpp"
 "../ipDeviceCommon/*.h"
)

set(COMPRESSION_SOURCES ${COMPRESSION_SOURCES} ${LZ4_DIR}/lz4.h ${LZ4_DIR}/lz4.c)

add_library(${PROJECT_NAME} STATIC ${COMPRESSION_SOURCES})

include_directories(${PROJECT_NAME}
    ../../common
    ../ipDeviceCommon
    ../../third-party/easyloggingpp/src    
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

add_dependencies(realsense2-compression 
    libjpeg-turbo
    zlib
)

include_directories(${PROJECT_NAME}
    ${CMAKE_BINARY_DIR}/zlib/include
    ${CMAKE_BINARY_DIR}/libjpeg-turbo/include
    ${LZ4_INCLUDE_PATH}
)

if(WIN32)
    set(ZLIB_LIBRARY optimized ${CMAKE_BINARY_DIR}/zlib/lib/zlibstatic.lib debug ${CMAKE_BINARY_DIR}/zlib/lib/zlibstaticd.lib)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE ${DEPENDENCIES}
        PRIVATE ${ZLIB_LIBRARY}
        PRIVATE ${CMAKE_BINARY_DIR}/libjpeg-turbo/lib/turbojpeg-static.lib
    )
else()
    target_link_libraries(${PROJECT_NAME}
        PRIVATE ${DEPENDENCIES}
        PRIVATE ${CMAKE_BINARY_DIR}/zlib/lib/libz.a
        PRIVATE ${CMAKE_BINARY_DIR}/libjpeg-turbo/lib/libturbojpeg.a
    )
endif()    

#set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Library")
#
#install(TARGETS ${PROJECT_NAME}
#    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#)
