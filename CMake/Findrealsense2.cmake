# Find package module for Realsense2
#
# This module will populate the following variables:
#  REALSENSE2_FOUND - True if the system found all required path for compiling and linking with realsense2
#  REALSENSE2_INCLUDE_DIRS - The Realsense2 include directories
#  REALSENSE2_LIBRARIES - The libraries needed to use Realsense2
#  REALSENSE2_DEFINITIONS - Compiler switches required for using Realsense2
#  On Windows: REALSENSE2_RUNTIME -The location of the runtime library

set (REALSENSE2_ROOT_DIR $ENV{REALSENSE2_PATH})

if(NOT REALSENSE2_ROOT_DIR)
    #If there is no environment variable for realsense, try to use common locations
    if(WIN32)
        set(REALSENSE2_ROOT_DIR "$ENV{ProgramFiles}\\Intel RealSense SDK 2.0")
    endif()
endif()

find_path(REALSENSE2_INCLUDE_DIR
    NAMES 
        librealsense2
    PATHS 
         ${REALSENSE2_ROOT_DIR}
    PATH_SUFFIXES
        include
    DOC 
        "The Realsense2 include directory"
)

if(WIN32)
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(TRAGET_ARCH "x64")
    else()
        set(TRAGET_ARCH "x86")
    endif()
endif()

find_library(REALSENSE2_LIBRARY 
    NAMES 
        realsense2
    PATHS 
         ${REALSENSE2_ROOT_DIR}/lib
         ${REALSENSE2_ROOT_DIR}/lib/${TRAGET_ARCH}
    DOC 
        "The Realsense2 library"
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBXML2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(REALSENSE2 DEFAULT_MSG REALSENSE2_INCLUDE_DIR REALSENSE2_LIBRARY)

if (REALSENSE2_FOUND)
    set(REALSENSE2_LIBRARIES ${REALSENSE2_LIBRARY} )
    set(REALSENSE2_INCLUDE_DIRS ${REALSENSE2_INCLUDE_DIR} )
    #set(REALSENSE2_DEFINITIONS )
    if(WIN32)
        set(REALSENSE2_RUNTIME ${REALSENSE2_ROOT_DIR}\\bin\\${TRAGET_ARCH})
    endif()
endif()

mark_as_advanced(REALSENSE2_ROOT_DIR REALSENSE2_INCLUDE_DIR REALSENSE2_LIBRARY TRAGET_ARCH)

