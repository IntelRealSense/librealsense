# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.1.0)

# Note: we use 'pyrs_dds_server' because Python doesn't deal with '-' in module
# names very well... see the PROJECT_LABEL below...
project( pyrs_dds_server )

# To make sure pybind11 is already built, we set pyrealsense2 as a dependency (but it's not really)
set( DEPENDENCIES pyrealsense2 )

# In order for the external project clone to occur during cmake configure step(rather than during compilation, as would normally happen),
# we copy the external project declaration to the build folder and then execute it
configure_file(${CMAKE_SOURCE_DIR}/third-party/pybind11/CMakeLists.txt ${CMAKE_BINARY_DIR}/external-projects/pybind11/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/pybind11" RESULT_VARIABLE configure_ret
    )
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/external-projects/pybind11" RESULT_VARIABLE build_ret
    )
if(configure_ret OR build_ret)
    message( FATAL_ERROR "Failed to build pybind11")
endif()
    
set( PYBIND11_CPP_STANDARD -std=c++11 )

# Force Pybind11 not to share pyrealsense2 resources with other pybind modules.
# With this definition we force the ABI version to be unique and not risk crashes on different modules.
# (workaround for RS5-10582; see also https://github.com/pybind/pybind11/issues/2898)
add_definitions( -DPYBIND11_COMPILER_TYPE="_pyrs_dds_server_abi" )

include_directories( ${CMAKE_BINARY_DIR}/third-party/pybind11/include )

set( PYRS_DDS_SERVER_FILES
    python.cpp
    python.hpp
    )

pybind11_add_module( ${PROJECT_NAME} SHARED ${PYRS_DDS_SERVER_FILES} )

# pybind11 version 2.6.1 require find_package to run after pybind11_add_module call,
# This way the pybind takes the python version set in PYTHON_EXECUTABLE variable.
if (CMAKE_VERSION VERSION_LESS 3.12)
  find_package(PythonInterp REQUIRED)
  find_package(PythonLibs REQUIRED)
  set(PYTHON_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/${PROJECT_NAME}" CACHE PATH "Installation directory for Python bindings")
  set(CMAKECONFIG_PY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
else()
  find_package(Python REQUIRED COMPONENTS Interpreter Development)
  set(PYTHON_INSTALL_DIR "${Python_SITEARCH}/${PROJECT_NAME}" CACHE PATH "Installation directory for Python bindings")
  set(CMAKECONFIG_PY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
endif()

target_link_libraries( ${PROJECT_NAME} PRIVATE ${DEPENDENCIES} )
set_target_properties( ${PROJECT_NAME}
    PROPERTIES
        VERSION ${REALSENSE_VERSION_STRING}
        SOVERSION "${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}"
    )
set_target_properties( ${PROJECT_NAME}
    PROPERTIES
        FOLDER Tools/dds
        PROJECT_LABEL pyrs-dds-server
    )

target_include_directories( ${PROJECT_NAME} PRIVATE ${REPO_ROOT}/include )
include("${REPO_ROOT}/include/librealsense2/dds/dds-server.cmake")
#set(DDS_SERVER_FILES 
#    "${REPO_ROOT}/include/librealsense2/dds/dds-defines.h"
#    "${REPO_ROOT}/include/librealsense2/dds/dds-participant.h"
#    "${REPO_ROOT}/include/librealsense2/dds/dds-participant.cpp"
#    "${REPO_ROOT}/include/librealsense2/dds/dds-device-broadcaster.h"
#    "${REPO_ROOT}/include/librealsense2/dds/dds-device-broadcaster.cpp"
#    "${REPO_ROOT}/include/librealsense2/dds/dds-device-server.h"
#    "${REPO_ROOT}/include/librealsense2/dds/dds-device-server.cpp"
#    "${REPO_ROOT}/include/librealsense2/dds/dds-utilities.h"
#)

source_group( "DDS" FILES ${DDS_SERVER_FILES} )
# NOTE: this requires that your "project(...)" statement match the executable
target_link_libraries( ${PROJECT_NAME} PRIVATE dds )
target_sources( ${PROJECT_NAME} PRIVATE ${DDS_SERVER_FILES} )

include("${REPO_ROOT}/include/librealsense2/dds/topics/dds-topics.cmake")


include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")
include("${REPO_ROOT}/include/librealsense2/utilities/concurrency/CMakeLists.txt")
