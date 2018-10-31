cmake_minimum_required(VERSION 2.8)
project(libtm)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(LIBTM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBTM_INCLUDE_DIR "${LIBTM_ROOT}/libtm/include")
set(INFRA_INCLUDE_DIR "${LIBTM_ROOT}/libtm/src/infra")
set(LIBTM_SRC_DIR "${LIBTM_ROOT}/libtm/src")
set(LIBTM_RESOURCES_DIR "${LIBTM_ROOT}/resources")

include(versions.cmake)

add_definitions(-DBUILD_STATIC)

# Build resources (FW, Central, Controller binaries)
add_subdirectory(resources)
add_subdirectory(libtm)
add_subdirectory(tools)

message(STATUS "----------------------------------------------------------------------------")
message(STATUS "CMake Done")
message(STATUS "----------------------------------------------------------------------------")