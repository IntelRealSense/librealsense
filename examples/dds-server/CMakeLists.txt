# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(rs-dds-server)

set(SRC dds-server.cpp
        ../../src/dds/msg/devicesPubSubTypes.cpp # Condider moving it to a common folder
        ../../src/dds/msg/devices.cpp)
        
set(INC dds-server.h
        ../../src/dds/msg/devicesPubSubTypes.h
        ../../src/dds/msg/devices.h)

add_executable(${PROJECT_NAME} ${INC} ${SRC} )
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES} dds)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Examples)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})