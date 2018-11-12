#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesSensorControl)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

if(BUILD_GRAPHICAL_EXAMPLES)
    #sensor-control
    add_executable(rs-sensor-control api_how_to.h rs-sensor-control.cpp)
    target_link_libraries(rs-sensor-control ${DEPENDENCIES})
    include_directories(rs-sensor-control)
    set_target_properties (rs-sensor-control PROPERTIES FOLDER Examples)
    install(TARGETS rs-sensor-control RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
