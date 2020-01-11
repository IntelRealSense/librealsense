#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealSenseLatencyToolExample)

add_executable(rs-latency-tool rs-latency-tool.cpp latency-detector.h ../cv-helpers.hpp)
set_property(TARGET rs-latency-tool PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-latency-tool ${DEPENDENCIES})
set_target_properties (rs-latency-tool PROPERTIES
	FOLDER "Examples/OpenCV"
)

install(
	TARGETS

	rs-latency-tool

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
