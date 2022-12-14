# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesMeasure)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-measure rs-measure.cpp ../example.hpp)
    set_property(TARGET rs-measure PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-measure ${DEPENDENCIES} Threads::Threads)
    include_directories(../)
    set_target_properties (rs-measure PROPERTIES FOLDER "Examples")

    install(TARGETS rs-measure RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
