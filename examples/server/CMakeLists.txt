# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesServer)

add_executable(rs-server rs-server.cpp ../../third-party/easyloggingpp/src/easylogging++.cc)
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)
set_property(TARGET rs-server PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-server realsense2-net ${DEPENDENCIES})
include_directories(rs-server ../../third-party/tclap/include ../../third-party/easyloggingpp/src)
set_target_properties (rs-server PROPERTIES FOLDER "Examples")

install(TARGETS rs-server RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
