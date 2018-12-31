#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealSenseDNNExample)

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

add_executable(rs-dnn rs-dnn.cpp ../cv-helpers.hpp)
target_link_libraries(rs-dnn ${DEPENDENCIES})
set_target_properties (rs-dnn PROPERTIES
	FOLDER "Examples/OpenCV"
)

set(PROTOTEXT_FILE "MobileNetSSD_deploy.prototxt")
set(PROTOTEXT_URL "https://raw.githubusercontent.com/chuanqi305/MobileNet-SSD/f5d072ccc7e3dcddaa830e9805da4bf1000b2836/MobileNetSSD_deploy.prototxt")
set(PROTOTEXT_PATH "${CMAKE_CURRENT_BINARY_DIR}/${PROTOTEXT_FILE}")

set(MODEL_FILE "MobileNetSSD_deploy.caffemodel")
set(MODEL_URL "https://drive.google.com/uc?export=download&id=0B3gersZ2cHIxRm5PMWRoTkdHdHc")
set(MODEL_PATH "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_FILE}")

if(NOT EXISTS "${PROTOTEXT_PATH}")
	message("Downloading ${PROTOTEXT_FILE} into ${CMAKE_CURRENT_BINARY_DIR}")
    file(DOWNLOAD "${PROTOTEXT_URL}" "${PROTOTEXT_PATH}")
endif()

if(NOT EXISTS "${MODEL_PATH}")
	message("Downloading ${MODEL_FILE} into ${CMAKE_CURRENT_BINARY_DIR}")
    file(DOWNLOAD "${MODEL_URL}" "${MODEL_PATH}")
endif()

install(
	TARGETS

	rs-dnn

	RUNTIME DESTINATION
	${CMAKE_INSTALL_PREFIX}/bin
)
