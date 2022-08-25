# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseServer)

add_executable(rs-server rs-server.cpp ${CMAKE_SOURCE_DIR}/third-party/easyloggingpp/src/easylogging++.cc)
add_executable(rs-server-upgrade rs-server-upgrade.cpp)

add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)
set_property(TARGET rs-server PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-server realsense2-net ${DEPENDENCIES})
include_directories(rs-server 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/third-party/tclap/include 
    ${CMAKE_SOURCE_DIR}/third-party/easyloggingpp/src 
    ${CMAKE_SOURCE_DIR}/third-party/cpp-httplib
)
set_target_properties (rs-server PROPERTIES FOLDER "Tools")

install(TARGETS rs-server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES "${CMAKE_SOURCE_DIR}/tools/rs-server/scripts/start-rs-server.sh"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES "${CMAKE_SOURCE_DIR}/tools/rs-server/scripts/rs-server.service"
    DESTINATION /lib/systemd/system/
)


