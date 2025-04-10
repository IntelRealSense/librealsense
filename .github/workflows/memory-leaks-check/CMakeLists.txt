# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2025 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(mem-leak-test)

add_executable( ${PROJECT_NAME} mem-leak-test.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
target_link_libraries(  ${PROJECT_NAME} ${DEPENDENCIES} )
set_target_properties(  ${PROJECT_NAME} PROPERTIES FOLDER "Examples" )
install( TARGETS  ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

