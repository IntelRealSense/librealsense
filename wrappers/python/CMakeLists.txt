#  minimum required cmake version: 3.8.0
cmake_minimum_required(VERSION 3.8.0)

project(RealsensePythonWrappers)

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

add_subdirectory(third_party/pybind11)

pybind11_add_module(pyrealsense2 SHARED python.cpp)
target_link_libraries(pyrealsense2 PRIVATE ${DEPENDENCIES})
set_target_properties(pyrealsense2 PROPERTIES VERSION
        ${REALSENSE_VERSION_STRING} SOVERSION ${REALSENSE_VERSION_MAJOR})
set_target_properties(pyrealsense2 PROPERTIES FOLDER Wrappers/python)
install(TARGETS pyrealsense2 EXPORT realsense2Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(RAW_RS_CPP ../../src/backend.cpp ../../src/win/win-helpers.cpp ../../src/win/win-uvc.cpp pybackend_extras.cpp pybackend.cpp
           ../../src/win/win-usb.cpp ../../src/win/win-backend.cpp ../../src/linux/backend-v4l2.cpp ../../src/linux/backend-hid.cpp ../../src/types.cpp
           ../../src/win/win-hid.cpp ../../src/archive.cpp ../../src/log.cpp ../../src/types.cpp ../../third-party/easyloggingpp/src/easylogging++.cc)
set(RAW_RS_HPP ../../src/backend.h ../../src/win/win-helpers.h ../../src/win/win-uvc.h ../../src/linux/backend-v4l2.h ../../src/linux/backend-hid.h
           ../../src/win/win-usb.h ../../src/win/win-hid.h ../../src/win/win-backend.h ../../src/archive.h pybackend_extras.h
           ../../src/types.h ../../third-party/easyloggingpp/src/easylogging++.h)

pybind11_add_module(pybackend2 SHARED ${RAW_RS_CPP} ${RAW_RS_HPP})
target_link_libraries(pybackend2 PRIVATE ${LIBUSB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(pybackend2 PROPERTIES
                        VERSION     ${REALSENSE_VERSION_STRING}
                        SOVERSION   ${REALSENSE_VERSION_MAJOR})
set_target_properties(pybackend2 PROPERTIES FOLDER Wrappers/python)
include_directories(pybackend2 ../../include)
install(TARGETS pybackend2
        EXPORT realsense2Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
