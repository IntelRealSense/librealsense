#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealSenseDlibFaceExample)

add_executable(rs-face-dlib
	rs-face-dlib.cpp
	../rs_frame_image.h
	markup_68.h
	validate_face.h
	render_face.h
)
set_property(TARGET rs-face-dlib PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-face-dlib ${DEPENDENCIES})
set_target_properties (rs-face-dlib PROPERTIES
	FOLDER "Examples/dlib"
)

install(
	TARGETS

	rs-face-dlib

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
