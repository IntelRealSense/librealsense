# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.


project(rs-dds-sniffer)

add_executable(${PROJECT_NAME} dds-sniffer.cpp)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)

add_dependencies(${PROJECT_NAME} dds)
include_directories(../../../third-party/tclap/include ${CMAKE_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} dds)

set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER Tools/dds)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

