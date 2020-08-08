#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseOpenVINOSamples)

# Set dependencies OpenVINO dependencies
find_package(InferenceEngine REQUIRED)
find_package(ngraph REQUIRED)
get_property(deps VARIABLE PROPERTY DEPENDENCIES)
set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES} ${NGRAPH_LIBRARIES})
include_directories( . )
include_directories( ${InferenceEngine_INCLUDE_DIRS} )
include_directories( $<TARGET_PROPERTY:ngraph::ngraph,INTERFACE_INCLUDE_DIRECTORIES> )

# The rs-vino directory includes additional classes and helpers that need to be included
set(OPENVINO_FILES
	../rs-vino/base-detection.cpp
	../rs-vino/base-detection.h
	../rs-vino/object-detection.cpp
	../rs-vino/object-detection.h
	../rs-vino/detected-object.cpp
	../rs-vino/detected-object.h
	../rs-vino/openvino-helpers.h
)
# And they make use of ELPP (EasyLogging++):
include_directories( ../../third-party/easyloggingpp/src )
set( ELPP_FILES
    ../../../third-party/easyloggingpp/src/easylogging++.cc
    ../../../third-party/easyloggingpp/src/easylogging++.h
)
# The individual examples should use the above like this:
#     add_executable( <exe> <source files ...> ${OPENVINO_FILES} ${ELPP_FILES} )
#     source_group("OpenVINO" FILES ${OPENVINO_FILES})
#     source_group("EasyLogging++" FILES ${ELPP_FILES})

# Finally, OpenVINO model files are not included in our distribution. Define a function for easy
# downloading at CMake time:
include(dl_vino_model.cmake)

# List all the specific examples for OpenVINO
add_subdirectory(face)
add_subdirectory(dnn)
