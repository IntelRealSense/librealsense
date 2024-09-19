# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project( rs-measure )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(${PROJECT_NAME} rs-measure.cpp ../example.hpp)
    set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
    target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES} Threads::Threads)
    include_directories(../)
    set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER "Examples")

    using_easyloggingpp( ${PROJECT_NAME} SHARED )

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
