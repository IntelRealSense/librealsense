#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesEnumerateDevices)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

# enumerate-devices
add_executable(rs-enumerate-devices rs-enumerate-devices.cpp)
target_link_libraries(rs-enumerate-devices ${DEPENDENCIES})
include_directories(rs-enumerate-devices ../../third-party/tclap/include)
set_target_properties (rs-enumerate-devices PROPERTIES
    FOLDER Tools
)

install(
    TARGETS

    rs-enumerate-devices

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
