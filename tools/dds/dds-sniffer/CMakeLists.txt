# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.


project(rs-dds-sniffer)

set(SRC rs-dds-sniffer.cpp ${REPO_ROOT}/include/librealsense2/dds/dds-participant.cpp)
set(INC rs-dds-sniffer.h   ${REPO_ROOT}/include/librealsense2/dds/dds-participant.h)

add_executable(${PROJECT_NAME} ${INC} ${SRC} ${ELPP_SOURCES})

include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")
include_directories(${REPO_ROOT}/third-party/tclap/include ${ELPP_INCLUDES} ${CMAKE_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} dds ${DEPENDENCIES})

set_target_properties (${PROJECT_NAME} PROPERTIES
                       FOLDER Tools/dds
                       CXX_STANDARD 11)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

