#  minimum required cmake version: 3.8.0
cmake_minimum_required(VERSION 3.8.0)

project(RealsenseToolsTerminal)

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

# terminal
add_executable(rs-terminal rs-terminal.cpp auto-complete.cpp auto-complete.h)
if(WIN32 OR ANDROID)
    target_link_libraries(rs-terminal ${DEPENDENCIES})
else()
    target_link_libraries(rs-terminal -lpthread ${DEPENDENCIES})
endif()
include_directories(rs-terminal ../../common ../../third-party ../../third-party/tclap/include)
set_target_properties (rs-terminal PROPERTIES
    FOLDER Tools
)

install(
    TARGETS

    rs-terminal

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
)

