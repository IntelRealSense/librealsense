# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsTerminal)

add_executable(rs-terminal rs-terminal.cpp auto-complete.cpp auto-complete.h)
set_property(TARGET rs-terminal PROPERTY CXX_STANDARD 11)
if(WIN32 OR ANDROID)
    target_link_libraries(rs-terminal ${DEPENDENCIES})
else()
    target_link_libraries(rs-terminal -lpthread ${DEPENDENCIES})
endif()
include_directories(rs-terminal ../../common ../../third-party ../../third-party/tclap/include)
set_target_properties (rs-terminal PROPERTIES
    FOLDER Tools
)

install(
    TARGETS

    rs-terminal

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

