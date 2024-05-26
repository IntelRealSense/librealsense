cmake_minimum_required(VERSION 3.8)

project(RealsenseOpenVINOSamples)

# Add INTEL_OPENVINO_DIR as an option -- this needs to be configured in CMake
set(INTEL_OPENVINO_DIR "" CACHE PATH "The path to the OpenVINO Toolkit installation")

if( NOT DEFINED INTEL_OPENVINO_DIR  OR  NOT IS_DIRECTORY ${INTEL_OPENVINO_DIR} )
	message( FATAL_ERROR "Invalid OpenVINO directory specified with INTEL_OPENVINO_DIR" )
endif()

if (NOT BUILD_WITH_CPU_EXTENSIONS)
    message( STATUS "Disabling CPU extensions for OpenVINO" )
    set(ENABLE_AVX2    OFF)
    set(ENABLE_AVX512F OFF)
endif()
           
# Set dependencies, including a CPU extension
set(IE_ROOT_DIR "${INTEL_OPENVINO_DIR}/inference_engine")
include(${INTEL_OPENVINO_DIR}/inference_engine/share/InferenceEngineConfig.cmake)
get_property(deps VARIABLE PROPERTY DEPENDENCIES)

include(check_vino_version.cmake)
check_vino_version()

if(OPENVINO2019)
    set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES} ie_cpu_extension)
elseif(OPENVINO_NGRAPH)
    include(${INTEL_OPENVINO_DIR}/deployment_tools/ngraph/cmake/ngraphConfig.cmake)
    set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES} ${NGRAPH_LIBRARIES})
else()
    set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES})
endif()

include_directories( . )
include_directories( ${InferenceEngine_INCLUDE_DIRS} )

if(OPENVINO2019)
# We need additional access to ext_list.hpp, for CPU extension support:
    include_directories( "${IE_ROOT_DIR}/src/extension" )
elseif(OPENVINO_NGRAPH)
    include_directories( "${INTEL_OPENVINO_DIR}/deployment_tools/ngraph/include" )
endif()

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

if(OPENVINO2019)
    set(OPENVINO_FILES ${OPENVINO_FILES} "${IE_ROOT_DIR}/src/extension/ext_list.hpp")
endif()

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
