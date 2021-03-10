#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealSenseKinfuExample)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

set(DEPENDENCIES glfw ${OPENGL_LIBRARIES} ${DEPENDENCIES})

add_executable(rs-kinfu rs-kinfu.cpp ../../../examples/example.hpp)
set_property(TARGET rs-kinfu PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-kinfu ${DEPENDENCIES})
include_directories(rs-kinfu ../../../examples)
set_target_properties (rs-kinfu PROPERTIES
	FOLDER "Examples/OpenCV"
)

install(
	TARGETS

	rs-kinfu

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
