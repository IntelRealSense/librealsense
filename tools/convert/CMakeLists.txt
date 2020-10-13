# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsConvert)
set(RS_TARGET rs-convert)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${RS_TARGET} rs-convert.cpp
    converter.hpp
    converters/converter-bin.hpp
    converters/converter-csv.hpp
    converters/converter-ply.hpp
    converters/converter-png.hpp
    converters/converter-raw.hpp
)
set_property(TARGET ${RS_TARGET} PROPERTY CXX_STANDARD 11)
target_link_libraries(${RS_TARGET} ${DEPENDENCIES} Threads::Threads)
include_directories(${RS_TARGET} ../../common ../../third-party ../../third-party/tclap/include)

set_target_properties (${RS_TARGET} PROPERTIES
    FOLDER "Tools"
)

install(
    TARGETS
    ${RS_TARGET}
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
