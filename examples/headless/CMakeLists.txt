# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

project(RealsenseExamplesHeadless)

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

# headless
add_executable(rs-headless rs-headless.cpp)
target_link_libraries(rs-headless ${DEPENDENCIES})
include_directories(rs-headless ../../common ../../third-party ../../third-party/tclap/include)
set_target_properties (rs-headless PROPERTIES
    FOLDER "Examples"
)
    
install(
    TARGETS

    rs-headless

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
)
