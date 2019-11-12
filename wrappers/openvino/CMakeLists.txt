#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseOpenVINOSamples)

# Add INTEL_OPENVINO_DIR as an option -- this needs to be configured in CMake
set(INTEL_OPENVINO_DIR "" CACHE PATH "The path to the OpenVINO Toolkit installation")

if( NOT DEFINED INTEL_OPENVINO_DIR  OR  NOT IS_DIRECTORY ${INTEL_OPENVINO_DIR} )
	message( FATAL_ERROR "Invalid OpenVINO directory specified with INTEL_OPENVINO_DIR" )
endif()

# Set dependencies, including a CPU extension
set(IE_ROOT_DIR "${INTEL_OPENVINO_DIR}/inference_engine")
include(${INTEL_OPENVINO_DIR}/inference_engine/share/InferenceEngineConfig.cmake)
get_property(deps VARIABLE PROPERTY DEPENDENCIES)
set(DEPENDENCIES ${deps} ${InferenceEngine_LIBRARIES} ie_cpu_extension)
include_directories( . )
include_directories( ${InferenceEngine_INCLUDE_DIRS} )
# We need additional access to ext_list.hpp, for CPU extension support:
include_directories( ${IE_ROOT_DIR}/src/extension )

# List all the specific examples for OpenVINO
add_subdirectory(face)
