# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.

project(rs-dds-sniffer)

set(SRC rs-dds-sniffer.cpp)
set(INC rs-dds-sniffer.h)

add_executable(${PROJECT_NAME} ${INC} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE realdds tclap )
set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER Tools/dds
    CXX_STANDARD 11
    )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

using_easyloggingpp( ${PROJECT_NAME} )

