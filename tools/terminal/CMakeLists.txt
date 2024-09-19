# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)
project( rs-terminal )

include_directories(
        ../../common
        ../../third-party
        )
set(RS_TERMINAL_SOURCES
        rs-terminal.cpp
        auto-complete.cpp
        auto-complete.h
        ../../common/parser.hpp
    )
add_executable(${PROJECT_NAME} ${RS_TERMINAL_SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
if(WIN32 OR ANDROID)
    target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES} tclap )
else()
    target_link_libraries(${PROJECT_NAME} -lpthread ${DEPENDENCIES} tclap )
endif()
set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER Tools
)

using_easyloggingpp( ${PROJECT_NAME} SHARED )

install(
    TARGETS

    ${PROJECT_NAME}

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)

