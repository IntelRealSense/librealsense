# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(rs-convert)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} rs-convert.cpp
    converter.hpp
    converter.cpp
    converters/converter-bin.hpp
    converters/converter-csv.hpp
    converters/converter-csv.cpp
    converters/converter-ply.hpp
    converters/converter-png.hpp
    converters/converter-raw.hpp
)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
target_link_libraries( ${PROJECT_NAME} ${DEPENDENCIES} Threads::Threads tclap )
#include_directories(../../common ../../third-party)

set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER "Tools"
)

using_easyloggingpp( ${PROJECT_NAME} SHARED )

install(
    TARGETS
    ${PROJECT_NAME}
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
