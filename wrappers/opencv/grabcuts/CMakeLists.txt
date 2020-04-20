#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealSenseGrabCutsExample)

add_executable(rs-grabcuts rs-grabcuts.cpp ../cv-helpers.hpp)
set_property(TARGET rs-grabcuts PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-grabcuts ${DEPENDENCIES})
set_target_properties (rs-grabcuts PROPERTIES
	FOLDER "Examples/OpenCV"
)

install(
	TARGETS

	rs-grabcuts

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
