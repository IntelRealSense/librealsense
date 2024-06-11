cmake_minimum_required(VERSION 3.8)

project(RealSenseImShowExample)

add_executable(rs-imshow rs-imshow.cpp)
set_property(TARGET rs-imshow PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-imshow ${DEPENDENCIES})
set_target_properties (rs-imshow PROPERTIES
	FOLDER "Examples/OpenCV"
)

install(
	TARGETS

	rs-imshow

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
