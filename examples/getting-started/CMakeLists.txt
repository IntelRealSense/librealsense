#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesGettingStarted)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Make project require C++11
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

########### Find RealSense2

# Use provided FindLibrealsense2.cmake to find the include dir and lib file
set(FINDLIBRS_PATH "../../CMake") # Change this to match correct location of FindLibrealsense2.cmake

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${FINDLIBRS_PATH})
find_package(realsense2 REQUIRED)
if(NOT REALSENSE2_FOUND)
    message(FATAL " Package realsense2 was not found")
endif()

###########


# getting-started
add_definitions(${REALSENSE2_DEFINITIONS})
add_executable(rs-getting-started rs-getting-started.cpp)
include_directories(rs-getting-started ${REALSENSE2_INCLUDE_DIR})
target_link_libraries(rs-getting-started ${REALSENSE2_LIBRARIES})
set_target_properties (rs-getting-started PROPERTIES FOLDER Examples)

# Post Build script to copy realsense2.dll next to this project's exe
if(WIN32)
if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
    endif()
    # Set PROJECT_BINARY_OUTPUT to the path of the binary output of this application
    set(PROJECT_BINARY_OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}) 
    # For standalone project you might need: set(PROJECT_BINARY_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})

    message(STATUS "Adding Post build script to copy realsense2.dll to project's binary folder")
    message(STATUS "Will try to copy from ${REALSENSE2_RUNTIME} to ${PROJECT_BINARY_OUTPUT}")
    add_custom_command(
        TARGET rs-getting-started POST_BUILD                       # Adds a post-build event to rs-getting-started
        COMMAND ${CMAKE_COMMAND} -E copy_if_different              # which executes "cmake - E copy_if_different..."
                ${REALSENSE2_RUNTIME}/realsense2.dll                                  # <--this is in-file
                ${PROJECT_BINARY_OUTPUT})   # <--this is out-file path
endif()

install(
    TARGETS
        rs-getting-started
    RUNTIME DESTINATION
        ${CMAKE_INSTALL_PREFIX}/bin
)
