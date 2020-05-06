#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealSenseDepthFilterExample)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")

add_executable(rs-depth-filter rs-depth-filter.cpp downsample.h downsample.cpp)
include_directories(rs-depth-filter ../../../common)
target_link_libraries(rs-depth-filter ${DEPENDENCIES})
set_target_properties (rs-depth-filter PROPERTIES
	FOLDER "Examples/OpenCV"
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/camera-settings.json
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

install(
	TARGETS

	rs-depth-filter

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
