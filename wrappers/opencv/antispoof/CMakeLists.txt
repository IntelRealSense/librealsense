#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealSenseAntispoofExample)

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

add_executable(rs-antispoof rs-antispoof.cpp ../cv-helpers.hpp)
target_link_libraries(rs-antispoof ${DEPENDENCIES})
set_target_properties (rs-antispoof PROPERTIES
	FOLDER "Examples/OpenCV"
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data/haarcascade_frontalface_alt2.xml
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

install(
	TARGETS

	rs-antispoof

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)