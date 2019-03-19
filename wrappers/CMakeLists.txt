#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseWrappers)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

set(DEPENDENCIES realsense2)

if (BUILD_PYTHON_BINDINGS OR BUILD_PYTHON_DOCS)
  add_subdirectory(python)
endif()

if (BUILD_NODEJS_BINDINGS)
  add_subdirectory(nodejs)
endif()

if (BUILD_CV_EXAMPLES)
  add_subdirectory(opencv)
endif()

if(BUILD_MATLAB_BINDINGS)
    add_subdirectory(matlab)
endif()

if (BUILD_PCL_EXAMPLES)
  add_subdirectory(pcl)
endif()

if(BUILD_UNITY_BINDINGS)
	if(NOT WIN32)
		message( FATAL_ERROR "BUILD_UNITY_BINDINGS is supported only for Windows OS" )
	endif()
	if(NOT BUILD_CSHARP_BINDINGS)
		message( WARNING "Enabling BUILD_CSHARP_BINDINGS and set DOTNET_VERSION_LIBRARY to 3.5 since BUILD_UNITY_BINDINGS is enabled" )
	endif()
	SET(BUILD_CSHARP_BINDINGS ON)
	SET(DOTNET_VERSION_LIBRARY 3.5)
endif()

if(BUILD_CSHARP_BINDINGS)
	if(NOT WIN32)
		message( WARNING "BUILD_CSHARP_BINDINGS is supported only for Windows OS" )
	endif()
	if(NOT BUILD_SHARED_LIBS)
		message( FATAL_ERROR "BUILD_SHARED_LIBS flag must be enabled when generating CSharp bindings" )
	endif()
    add_subdirectory(csharp)
endif()

if(BUILD_OPENNI2_BINDINGS)
	add_subdirectory(openni2)
endif()
