cmake_minimum_required(VERSION 2.8.3)

set(HEADER_FILES_CPP_COMMON
        ${CMAKE_CURRENT_LIST_DIR}/include/ros/cpp_common_decl.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ros/datatypes.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ros/debug.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ros/exception.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ros/header.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ros/macros.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ros/platform.h
        ${CMAKE_CURRENT_LIST_DIR}/include/ros/types.h
        )

set(SOURCE_FILES_CPP_COMMON
        ${CMAKE_CURRENT_LIST_DIR}/src/debug.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/header.cpp
        )
