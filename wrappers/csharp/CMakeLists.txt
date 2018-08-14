cmake_minimum_required( VERSION 3.8.0 )

project(LANGUAGES CSharp)
include(CSharpUtilities)
set(CMAKE_CSharp_FLAGS "/langversion:6")

add_subdirectory(Intel.RealSense)
add_subdirectory(cs-tutorial-1-depth)
add_subdirectory(cs-tutorial-2-capture)
add_subdirectory(cs-tutorial-3-processing)
add_subdirectory(cs-tutorial-4-software-dev)