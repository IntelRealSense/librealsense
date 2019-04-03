# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.

cmake_minimum_required(VERSION 2.8.9)

project(realsense2-gl)

set(REALSENSE_GL_CPP
    synthetic-stream-gl.h
    synthetic-stream-gl.cpp
    yuy2rgb-gl.h
    yuy2rgb-gl.cpp
    pointcloud-gl.h
    pointcloud-gl.cpp
    rs-gl.cpp
    realsense-gl.def
    ../../include/librealsense2-gl/rs_processing_gl.h
    ../../include/librealsense2-gl/rs_processing_gl.hpp
    ../../third-party/glad/glad.c
    ../../common/opengl3.cpp
    ../../common/opengl3.h
    pc-shader.cpp
    pc-shader.h
    camera-shader.cpp
    camera-shader.h
    upload.h
    upload.cpp
    colorizer-gl.h
    colorizer-gl.cpp
    align-gl.h
    align-gl.cpp
    ${LZ4_DIR}/lz4.h
    ${LZ4_DIR}/lz4.c
)

if (${BUILD_SHARED_LIBS} AND ${BUILD_EASYLOGGINGPP})
    list(APPEND REALSENSE_GL_CPP ../../third-party/easyloggingpp/src/easylogging++.cc)
endif()

source_group("3rd Party" FILES
    ../../third-party/glad/glad.c)

source_group("API" FILES
    ../../include/librealsense2-gl/rs_processing_gl.h
    ../../include/librealsense2-gl/rs_processing_gl.hpp
    realsense-gl.def)

add_library(${PROJECT_NAME} ${REALSENSE_GL_CPP})
include_directories(${LZ4_DIR})

find_package(OpenGL)
if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "\n\n OpenGL package is missing!\n\n")
endif()

set(DEPENDENCIES realsense2 ${OPENGL_LIBRARIES} glfw)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/.."
    "${CMAKE_CURRENT_LIST_DIR}/../../include"
    "${CMAKE_CURRENT_LIST_DIR}/../../common/gl/"
    "${CMAKE_CURRENT_LIST_DIR}/../../common/"
    "${CMAKE_CURRENT_LIST_DIR}/../../third-party/glad"
    "${CMAKE_CURRENT_LIST_DIR}/../../third-party/glfw/include"
)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Library)
target_link_libraries(${PROJECT_NAME} PRIVATE ${DEPENDENCIES})

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/../cmake/realsense2")
