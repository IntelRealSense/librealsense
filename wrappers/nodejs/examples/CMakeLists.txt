# Copyright (c) 2017 Intel Corporation. All rights reserved.
# Use of this source code is governed by an Apache 2.0 license
# that can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.1.0)

project(RealsenseNodeJSExamples)

set(NODE_VERSOIN 6)

set(TOOL_PATH "node_modules/.bin/")
# Get OS
if(WIN32)
  set(OS win)
  set(TOOL_PATH "node_modules\\.bin\\")
elseif(APPLE)
  set(OS macos)
elseif(UNIX AND NOT APPLE)
  set(OS linux)
endif()

# Get CPU architecture
INCLUDE(cmake/TargetArch.cmake)
target_architecture(ARCH)
string(REPLACE "x86_64" "x64" ARCH ${ARCH})
string(REPLACE "i386" "x86" ARCH ${ARCH})

set(PKG_STR node${NODE_VERSOIN}-${OS}-${ARCH})

file(GLOB all_js_files "*.js")
# Compile examples
add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/node_modules/node-glfw-3/build/Release/glfw.node
  COMMAND npm install
  COMMAND ${TOOL_PATH}pkg-fetch node${NODE_VERSOIN} ${OS} ${ARCH}
  DEPENDS BUILD_NODE_ADDON package.json ${all_js_files}
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

add_custom_target(
  BUILD_NODE_EXAMPLES ALL
	DEPENDS ${PROJECT_SOURCE_DIR}/node_modules/node-glfw-3/build/Release/glfw.node
)

# Dist examples
file(GLOB nodejs_examples "nodejs-*.js")
foreach(example ${nodejs_examples})
  # Generate output file name
  get_filename_component(outfileName ${example} NAME_WE)
  set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfileName}")

  # Package
  add_custom_command(OUTPUT "${outfile}"
      COMMAND ${TOOL_PATH}pkg ${example} --targets=${PKG_STR} --out-path=${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS BUILD_NODE_EXAMPLES ${example}
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  set(packagedExamples ${packagedExamples} "${outfile}")
ENDFOREACH(example)

add_custom_target(
  DIST_NODE_EXAMPLES ALL
  DEPENDS ${packagedExamples}
)

add_custom_target(RealsenseNodeJSExamples DEPENDS DIST_NODE_EXAMPLES)

# Install packaged samples and required files
install(PROGRAMS ${packagedExamples} DESTINATION ${CMAKE_INSTALL_BINDIR})
# TODO: Use buildtype instead of hardcode of Release
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../build/Release/node_librealsense.node DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-glfw-3/build/Release/glfw.node DESTINATION ${CMAKE_INSTALL_BINDIR})
