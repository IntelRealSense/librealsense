# ubuntu 12.04 LTS cmake version 2.8.7
# ubuntu 14.04 LTS cmake version 2.8.12.2
# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

project(RealsenseUnitTests)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

set(DEPENDENCIES realsense)
if(WIN32)
else()
    list(APPEND DEPENDENCIES m ${LIBUSB1_LIBRARIES})
endif()

add_executable(F200-live-test unit-tests-live.cpp unit-tests-live-f200.cpp)
target_link_libraries(F200-live-test ${DEPENDENCIES})

add_executable(LR200-live-test unit-tests-live.cpp unit-tests-live-ds-common.cpp unit-tests-live-lr200.cpp)
target_link_libraries(LR200-live-test ${DEPENDENCIES})

add_executable(R200-live-test unit-tests-live.cpp unit-tests-live-ds-common.cpp unit-tests-live-r200.cpp)
target_link_libraries(R200-live-test ${DEPENDENCIES})

add_executable(SR300-live-test unit-tests-live.cpp unit-tests-live-sr300.cpp)
target_link_libraries(SR300-live-test ${DEPENDENCIES})

add_executable(ZR300-live-test unit-tests-live.cpp unit-tests-live-ds-common.cpp unit-tests-live-zr300.cpp)
target_link_libraries(ZR300-live-test ${DEPENDENCIES})

add_executable(offline-test unit-tests-offline.cpp)
target_link_libraries(offline-test ${DEPENDENCIES})

