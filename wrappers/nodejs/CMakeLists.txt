# Copyright (c) 2017 Intel Corporation. All rights reserved.
# Use of this source code is governed by an Apache 2.0 license
# that can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.1.0)

project(RealsenseNodeJSWrappers)

# Compile realsense addon
add_custom_command(
    OUTPUT ${PROJECT_SOURCE_DIR}/build/Release/node_librealsense.node
    COMMAND env GYP_DEFINES="vs_configuration=$<CONFIGURATION>" npm install
    DEPENDS realsense2 index.js binding.gyp package.json src/addon.cpp
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)
add_custom_target(
    BUILD_NODE_ADDON ALL
	DEPENDS ${PROJECT_SOURCE_DIR}/build/Release/node_librealsense.node
)
set_target_properties(BUILD_NODE_ADDON PROPERTIES FOLDER Wrappers/NodeJS)

add_custom_target(RealsenseNodeJSWrappers)
set_target_properties(RealsenseNodeJSWrappers PROPERTIES FOLDER Wrappers/NodeJS)

