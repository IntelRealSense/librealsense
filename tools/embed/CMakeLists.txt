#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsEmbed)
set(RS_TARGET rs-embed)

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

# embed
add_executable(${RS_TARGET} 
   rs-embed.cpp
   ${LZ4_DIR}/lz4.h
   ${LZ4_DIR}/lz4.c
)

include_directories(${RS_TARGET} ${LZ4_DIR} ../../third-party ../../third-party/tclap/include)

set_target_properties (${RS_TARGET} PROPERTIES
    FOLDER "Tools"
)

install(
    TARGETS
    ${RS_TARGET}
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
)
