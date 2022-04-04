# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
project(rs-dds-server)

set(SRC rs-dds-server.cpp
        dds-server.cpp
        ../../../src/dds/msg/devicesPubSubTypes.cpp # TODO Consider moving it to a common folder
        ../../../src/dds/msg/devices.cpp)
        
set(INC dds-server.h
        ../../../src/dds/msg/devicesPubSubTypes.h
        ../../../src/dds/msg/devices.h)

add_executable(${PROJECT_NAME} ${INC} ${SRC} )
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
include_directories(../../../third-party/tclap/include ${CMAKE_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES} dds)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Tools/dds)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})