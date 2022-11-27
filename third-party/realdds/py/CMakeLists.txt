# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.1.0)
project( pyrealdds )

# To make sure pybind11 is already built, we set pyrealsense2 as a dependency (but it's not really)
list(APPEND DEPENDENCIES realdds )

set( PYREALDDS_FILES
    pyrealdds.cpp
    )

pybind11_add_module( ${PROJECT_NAME} SHARED ${PYREALDDS_FILES} )

target_link_libraries( ${PROJECT_NAME} PRIVATE ${DEPENDENCIES} )
set_target_properties( ${PROJECT_NAME}
    PROPERTIES
        VERSION ${REALSENSE_VERSION_STRING}
        SOVERSION "${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}"
    )
set_target_properties( ${PROJECT_NAME}
    PROPERTIES
    FOLDER Library
        PROJECT_LABEL realdds-py
    )

include( "${REPO_ROOT}/third-party/utilities/include/utilities/easylogging/shared-init.cmake" )
