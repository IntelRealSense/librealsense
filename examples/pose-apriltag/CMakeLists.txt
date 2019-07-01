# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesPoseApriltag)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

if(BUILD_EXAMPLES)

    #add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib)
    find_library(apriltag-lib NAME apriltag PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

    if(TARGET apriltag)    
        add_executable(rs-pose-apriltag rs-pose-apriltag.cpp)
        target_link_libraries(rs-pose-apriltag ${DEPENDENCIES} apriltag)
        set_target_properties (rs-pose-apriltag PROPERTIES FOLDER Examples)
        install(TARGETS rs-pose-apriltag RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()

endif()
