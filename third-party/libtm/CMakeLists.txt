cmake_minimum_required(VERSION 2.8)
project(libtm)

include(cmake/os.cmake)

set(LIBTM_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(LIBTM_INCLUDE_DIR "${LIBTM_ROOT}/libtm/include")
set(LIBTM_INFRA_DIR "${LIBTM_ROOT}/libtm/infra")
set(LIBTM_SRC_DIR "${LIBTM_ROOT}/libtm/src")
set(LIBTM_RESOURCES_DIR "${LIBTM_ROOT}/resources")

message("${LIBTM_INCLUDE_DIR}")
include(versions.cmake)

# Build resources (FW, Central, Controller binaries)
add_subdirectory(resources)

set(LIB_TYPE "STATIC")
add_definitions(-DBUILD_STATIC)

add_subdirectory(libtm)

message("----------------------------------------------------------------------------")
message("CMake Done")
message("----------------------------------------------------------------------------")