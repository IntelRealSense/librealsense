# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
cmake_minimum_required( VERSION 3.15 )

#
# This tests whether we can link with realsense2-all and not have any missing external symbols.
# Without realsense2-all, we'd need to link with:
#     realsense2 realsense-file rsutils
# Or, if DDS was enabled:
#     realsense2 realsense-file rsutils fastcdr fastrtps foonathan_memory realdds
# And the list gets longer if we add libraries.
#
# On Windows, no additional special libraries are needed.
# On Linux, we have other dependencies:
#     libusb udev
# These are not part of realsense2-all, even though we depend on them!
#

project( rs-all-client )

option( BUILD_SHARED_LIBS "Build using shared libraries" OFF )

if( WIN32 )
    # Take away the other configurations because they'd require we picked another link
    # directory and target... keep it simple...
    set( CMAKE_CONFIGURATION_TYPES "Release" )
endif()

add_executable( ${PROJECT_NAME} main.cpp )
set_target_properties( ${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 14
    MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>  # New in version 3.15; ignored in Linux
    )

target_include_directories( ${PROJECT_NAME} PRIVATE
    ../../../include
    ../../../third-party/rsutils/include
    )
target_link_directories( ${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/../Release )

target_link_libraries( ${PROJECT_NAME} PRIVATE realsense2-all )

if( NOT WIN32 )
    find_library( LIBUSB NAMES usb-1.0 )
    target_link_libraries( ${PROJECT_NAME} PRIVATE pthread ${LIBUSB} )
    if( NOT FORCE_RSUSB_BACKEND )
        target_link_libraries( ${PROJECT_NAME} PRIVATE udev )
    endif()
    if( BUILD_WITH_DDS )
        target_link_libraries( ${PROJECT_NAME} PRIVATE ssl crypto rt )
    endif()
endif()

