# Copyright (c) 2017 Intel Corporation. All rights reserved.
# Use of this source code is governed by an Apache 2.0 license
# that can be found in the LICENSE file.

cmake_minimum_required(VERSION 2.8.3)

project(RealsenseNodeJSWrappers)

add_custom_command(
    OUTPUT always_rebuild
    COMMAND cmake -E echo
)

add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/build/Release/node_librealsense.node
    COMMAND npm install
    DEPENDS realsense2 always_rebuild
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)
add_custom_target(
    BUILD_NODE_ADDON ALL
	DEPENDS ${PROJECT_SOURCE_DIR}/build/Release/node_librealsense.node
)

add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/examples/node_modules/node-glfw-2
    COMMAND npm install
    DEPENDS BUILD_NODE_ADDON always_rebuild
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/examples"
)

add_custom_target(
    BUILD_NODE_EXAMPLES ALL
	DEPENDS ${PROJECT_SOURCE_DIR}/examples/node_modules/node-glfw-2
)

add_custom_target(RealsenseNodeJSWrappers DEPENDS BUILD_NODE_EXAMPLES)
