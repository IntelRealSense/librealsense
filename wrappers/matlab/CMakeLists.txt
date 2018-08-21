#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseMatlabWrappers)

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

find_package(Matlab COMPONENTS MX_LIBRARY REQUIRED)

set(MATLAB_CPP librealsense_mex.cpp Factory.cpp)
set(MATLAB_H Factory.h MatlabParamParser.h rs2_type_traits.h types.h)
set(MATLAB_M context.m device.m roi_sensor.m sensor.m)

#TODO: There has to be more that needs to be done to make this work
matlab_add_mex(librealsense_mex ${MATLAB_CPP} ${MATLAB_H} ${DEPENDENCIES})

#TODO: additionally, the compiled mex file and all the .m files need to be copied into a folder called `+realsense` at the install site