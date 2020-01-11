# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 2.8)
project(libtm)

STRING(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+\\.[0-9]+" "\\1" LIBTM_VERSION_MAJOR "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" LIBTM_VERSION_MINOR "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" LIBTM_VERSION_PATCH "${HOST_VERSION}")
STRING(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" LIBTM_VERSION_BUILD "${HOST_VERSION}")

set (LIBTM_API_VERSION_MAJOR 10) # Major part of the device supported interface API version, updated upon an incompatible API change
set (LIBTM_API_VERSION_MINOR 0) # Minor part of the device supported interface API version, updated upon a backwards-compatible change

set(LIBVERSION ${LIBTM_VERSION_MAJOR}.${LIBTM_VERSION_MINOR}.${LIBTM_VERSION_PATCH})

# Retrieve Git branch name
execute_process(COMMAND git rev-parse --abbrev-ref HEAD
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)

if(GIT_BRANCH STREQUAL "")
    set (GIT_BRANCH "Branch_Unknown")
endif()

add_subdirectory(src)
