cmake_minimum_required(VERSION 3.12.0)
set (CMAKE_CXX_STANDARD 11)

if(POLICY CMP0091)
    # https://stackoverflow.com/a/56490614
    cmake_policy(SET CMP0091 NEW)
endif()

# The options need to be the same as Open3D's default
# If Open3D is configured and built with custom options, you'll also need to
# specify the same custom options.
option(STATIC_WINDOWS_RUNTIME "Use static (MT/MTd) Windows runtime" ON)
# This needs cmake_policy(SET CMP0091 NEW)
if (STATIC_WINDOWS_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

project(Open3D_RS_examples LANGUAGES CXX)

# Find installed Open3D, which exports Open3D::Open3D
if(WIN32)
    find_package(Open3D HINTS ${CMAKE_INSTALL_PREFIX}/CMake)
else()
    find_package(Open3D HINTS ${CMAKE_INSTALL_PREFIX}/lib/cmake)
endif()
if(NOT Open3D_FOUND OR Open3D_VERSION VERSION_LESS 0.12)
    message(FATAL_ERROR "Open3D v0.12+ not found, please use -DCMAKE_INSTALL_PREFIX=open3d_install_dir")
endif()

foreach(EXAMPLE RealSenseBagReader RealSenseRecorder)
    add_executable(${EXAMPLE} cpp/${EXAMPLE}.cpp)
    target_link_libraries(${EXAMPLE}  Open3D::Open3D)

    # On Windows, when BUILD_SHARED_LIBS, copy .dll to the executable directory
    if(WIN32)
        get_target_property(open3d_type Open3D::Open3D TYPE)
        if(open3d_type STREQUAL "SHARED_LIBRARY")
            message(STATUS "Will copy Open3D.dll to ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
            add_custom_command(TARGET ${EXAMPLE} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_INSTALL_PREFIX}/bin/Open3D.dll
                ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
        endif()
    endif()
endforeach()
