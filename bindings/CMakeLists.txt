# ubuntu 12.04 LTS cmake version 2.8.7
# ubuntu 14.04 LTS cmake version 2.8.12.2
# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

project(RealsenseBindings)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set(DEPENDENCIES realsense2)

option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)

if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(third_party/pybind11)
endif()

if(BUILD_PYTHON_BINDINGS)
    pybind11_add_module(pyrealsense2 python.cpp)
    target_link_libraries(pyrealsense2 ${DEPENDENCIES})
    set_target_properties(pyrealsense2 PROPERTIES FOLDER Bindings)
endif()

if(BUILD_PYTHON_BINDINGS)
    set(RAW_RS_CPP ../src/backend.cpp ../src/win/win-helpers.cpp ../src/win/win-uvc.cpp pybackend_extras.cpp pybackend.cpp
                   ../src/win/win-usb.cpp ../src/win/win-backend.cpp ../src/backend-v4l2.cpp ../src/backend-hid.cpp ../src/types.cpp
                   ../src/win/win-hid.cpp ../src/archive.cpp ../src/log.cpp ../src/types.cpp ../third_party/easyloggingpp/src/easylogging++.cc)
    set(RAW_RS_HPP ../src/backend.h ../src/win/win-helpers.h ../src/win/win-uvc.h ../src/backend-v4l2.h ../src/backend-hid.h
                   ../src/win/win-usb.h ../src/win/win-hid.h ../src/win/win-backend.h ../src/archive.h pybackend_extras.h
                   ../src/types.h ../third_party/easyloggingpp/src/easylogging++.h)
    pybind11_add_module(pybackend2 ${RAW_RS_CPP} ${RAW_RS_HPP})
    target_link_libraries(pybackend2 ${LIBUSB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    set_target_properties(pybackend2 PROPERTIES FOLDER Bindings)
endif()
