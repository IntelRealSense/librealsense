#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseExamplesMotion)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-motion rs-motion.cpp ../example.hpp ${LZ4_DIR}/lz4.c)
    target_link_libraries(rs-motion ${DEPENDENCIES})
    include_directories(rs-pointcloud ../ ${LZ4_DIR})
    set_target_properties (rs-motion PROPERTIES FOLDER Examples)
    install(TARGETS rs-motion RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
