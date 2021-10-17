cmake_minimum_required( VERSION 3.8.0 )

project(LANGUAGES CSharp)
include(CSharpUtilities)
set(CMAKE_CSharp_FLAGS "/langversion:6")

add_subdirectory(Intel.RealSense)
add_subdirectory(tutorial/depth)
add_subdirectory(tutorial/capture)
add_subdirectory(tutorial/processing)
add_subdirectory(tutorial/software-dev)
add_subdirectory(tutorial/d400-occ)
add_subdirectory(tutorial/load-json)

if(BUILD_NETWORK_DEVICE)
    add_subdirectory(Intel.RealSense.Net)
    add_subdirectory(tutorial/net-dev)
endif()