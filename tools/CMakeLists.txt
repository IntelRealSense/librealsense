#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseTools)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

# This parameter is meant for disabling graphical examples when building for
# save-to-disk targets.
option(BUILD_GRAPHICAL_EXAMPLES "Build graphical examples." ON)

if(BUILD_GRAPHICAL_EXAMPLES)
    include(${CMAKE_SOURCE_DIR}/CMake/opengl_config.cmake)
else()
    if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
        find_library(log-lib log)
        set(DEPENDENCIES realsense2 log)
    else()
        set(DEPENDENCIES realsense2)
    endif()
    if(NOT WIN32)
        list(APPEND DEPENDENCIES m ${LIBUSB1_LIBRARIES})
    endif()
endif()

add_subdirectory(terminal)
add_subdirectory(fw-logger)
add_subdirectory(enumerate-devices)
add_subdirectory(realsense-viewer)
add_subdirectory(data-collect)
add_subdirectory(depth-quality)
add_subdirectory(rosbag-inspector)
add_subdirectory(convert)
