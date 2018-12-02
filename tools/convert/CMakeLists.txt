#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsConvert)
set(RS_TARGET rs-convert)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

# convert
add_executable(${RS_TARGET} rs-convert.cpp
    converter.hpp
    converters/converter-bin.hpp
    converters/converter-csv.hpp
    converters/converter-ply.hpp
    converters/converter-png.hpp
    converters/converter-raw.hpp
)

target_link_libraries(${RS_TARGET} ${DEPENDENCIES} Threads::Threads)
include_directories(${RS_TARGET} ../../common ../../third-party ../../third-party/tclap/include)

set_target_properties (${RS_TARGET} PROPERTIES
    FOLDER "Tools"
)

install(
    TARGETS
    ${RS_TARGET}
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)
