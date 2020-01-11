#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseDlibSamples)

# Add DLIB_DIR as an option -- this needs to be configured in CMake
set(DLIB_DIR "" CACHE PATH "The path to the DLIB installation")

if( NOT DEFINED DLIB_DIR  OR  NOT IS_DIRECTORY ${DLIB_DIR} )
	message( FATAL_ERROR "Invalid DLIB directory specified with DLIB_DIR" )
endif()

# Actually make it part of the compilation
add_subdirectory(${DLIB_DIR} build)

# Add it as the source of includes??
set(DEPENDENCIES realsense2 dlib::dlib)
include_directories( ../../include )
include_directories( ${DLIB_DIR} )

# List all the specific examples for dlib
add_subdirectory(face)
