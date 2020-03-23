# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesAdvancedSoftwareDevice)

if(BUILD_LRS1_DEVICE_ADAPTOR)
    # Download Realsense1 from github and configure it with CMake
    include(ExternalProject)
    ExternalProject_Add(
        realsense1

        GIT_REPOSITORY "https://github.com/intelrealsense/librealsense.git"
        GIT_TAG "legacy"

        UPDATE_COMMAND ""
        PATCH_COMMAND ""

        SOURCE_DIR "third-party/realsense1/"
        CMAKE_ARGS -DCMAKE_CXX_STANDARD_LIBRARIES=${CMAKE_CXX_STANDARD_LIBRARIES}
                -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL}
                -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/lrs_install
                -DBUILD_SHARED_LIBS=FALSE
                -DBUILD_EXAMPLES=FALSE
                -DBUILD_UNIT_TESTS=FALSE
        
        TEST_COMMAND ""
        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/lrs_install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}realsense${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
    set_property(TARGET realsense1 PROPERTY FOLDER "Examples/Software Device")
    add_library(rs1 INTERFACE)
    target_include_directories(rs1 INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/third-party/realsense1/include>)
    target_link_libraries(rs1 INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/lrs_install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}realsense${CMAKE_STATIC_LIBRARY_SUFFIX})

    # Configure extension library
    if(BUILD_SHARED_LIBS)
        add_library(legacy-adaptor SHARED)
    else()
        add_library(legacy-adaptor STATIC)
    endif()
    target_sources(legacy-adaptor PRIVATE
        active_obj.cpp
        active_obj.hpp
        legacy_adaptor.h
        legacy_adaptor.hpp
        legacy_adaptor.cpp
        conversions.hpp
        legacy-adaptor.def
    )
    set_property(TARGET legacy-adaptor PROPERTY CXX_STANDARD 11)
    target_link_libraries(legacy-adaptor PRIVATE ${DEPENDENCIES} rs1)
    add_dependencies(legacy-adaptor realsense1)
    set_target_properties(legacy-adaptor PROPERTIES FOLDER "Examples/Software Device")

    # Configure capture-like example
    if(BUILD_GRAPHICAL_EXAMPLES)
        add_executable(rs-software-device-advanced rs-software-device-advanced.cpp ../example.hpp)
        include_directories(rs-software-device-advanced ../ ../../third-party/tclap/include)
        set_property(TARGET rs-software-device-advanced PROPERTY CXX_STANDARD 11)
        target_link_libraries(rs-software-device-advanced ${DEPENDENCIES} legacy-adaptor)
        set_target_properties (rs-software-device-advanced PROPERTIES FOLDER "Examples/Software Device")

        install(TARGETS rs-software-device-advanced RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()
