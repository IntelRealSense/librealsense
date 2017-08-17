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
endif()

set(DEPENDENCIES realsense2)
if(WIN32)
else()
    list(APPEND DEPENDENCIES m ${LIBUSB1_LIBRARIES})
endif()

add_executable(live-test unit-tests-live.cpp unit-tests-main.cpp unit-tests-common.h)
target_link_libraries(live-test ${DEPENDENCIES})

set_target_properties (live-test PROPERTIES
    FOLDER "Unit-Tests"
)

install(
    TARGETS

    live-test

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
)