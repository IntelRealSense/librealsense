cmake_minimum_required(VERSION 3.1.0)

project(RealSenseVinoDnnExample)


# OpenCV is required here. You can simply use the version that's supplied with
# OpenVINO (${INTEL_OPENVINO_DIR}/opencv/cmake).
if(NOT DEFINED OpenCV_DIR  OR  NOT IS_DIRECTORY ${OpenCV_DIR})
	set(OpenCV_DIR "" CACHE PATH "The path to the OpenCV Toolkit build directory")
	message( FATAL_ERROR "OpenVINO examples require OpenCV; specify OpenCV_DIR" )
endif()
find_package(OpenCV REQUIRED)
get_property(deps VARIABLE PROPERTY DEPENDENCIES)
set(DEPENDENCIES ${deps} ${OpenCV_LIBS})
include_directories( ../../opencv )


add_executable(rs-dnn-vino
	rs-dnn-vino.cpp
	${OPENVINO_FILES}
    ${ELPP_FILES}
)

source_group("OpenVINO" FILES ${OPENVINO_FILES})
source_group("EasyLogging++" FILES ${ELPP_FILES})

set_property(TARGET rs-dnn-vino PROPERTY CXX_STANDARD 11)
target_link_libraries(rs-dnn-vino ${DEPENDENCIES})
set_target_properties (rs-dnn-vino PROPERTIES
	FOLDER "Examples/openvino"
)

install(
	TARGETS

	rs-dnn-vino

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)


# Download model files -- these will go into build/wrappers/openvino/dnn, which is also where the sample
# is run from in Visual Studio
dl_vino_model( "README.txt"                    "789e144d6cafa379c8437d2a6860829b6d935a8d" )
dl_vino_model( "mobilenet-ssd.xml"             "483ba684dd53ba138550377075e928b2008418bb" )
dl_vino_model( "mobilenet-ssd.bin"             "3b687f0b9519b0d296c23d5f038a8623b5aeb49b" )
dl_vino_model( "mobilenet-ssd.labels"          "b11e85afe8cff3172ad9b62d904bbc8013b3ddbf" )
