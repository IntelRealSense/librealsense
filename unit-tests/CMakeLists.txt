#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

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

set (unit_tests_sources unit-tests-live.cpp unit-tests-post-processing.cpp unit-tests-main.cpp unit-tests-common.h)
# When compiling with static linkage, enable and activate librealsense-core classes unit-tests
if(NOT BUILD_SHARED_LIBS)
	set (unit_tests_sources ${unit_tests_sources} unit-tests-internal.cpp)
endif()

add_executable(live-test ${unit_tests_sources})
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
