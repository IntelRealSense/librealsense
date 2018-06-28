# Copyright (c) 2017 Intel Corporation. All rights reserved.
# Use of this source code is governed by an Apache 2.0 license
# that can be found in the LICENSE file.

#  minimum required cmake version: 3.8.0
cmake_minimum_required(VERSION 3.8.0)

project(RealsenseNodeJSExamples)

set(NODE_VERSOIN 8)

set(TOOL_PATH "./node_modules/.bin/")
# Get OS
if(WIN32)
  set(OS win)
  set(TOOL_PATH ".\\node_modules\\.bin\\")
elseif(APPLE)
  set(OS macos)
elseif(UNIX AND NOT APPLE)
  set(OS linux)
endif()

# Get CPU architecture
execute_process(COMMAND node -p "process.arch" OUTPUT_VARIABLE ARCH)
string(REPLACE "\n" "" ARCH ${ARCH})
string(REPLACE "x86_64" "x64" ARCH ${ARCH})
string(REPLACE "ia32" "x86" ARCH ${ARCH})

set(PKG_STR node${NODE_VERSOIN}-${OS}-${ARCH})

add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/node_modules/node-glfw-3/build/Release/glfw.node
  COMMAND npm install
  DEPENDS BUILD_NODE_ADDON package.json
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  COMMENT "Build node-glfw-3"
)

add_custom_target(
  BUILD_NODE_EXAMPLES ALL
  DEPENDS ${PROJECT_SOURCE_DIR}/node_modules/node-glfw-3/build/Release/glfw.node
)
set_target_properties(BUILD_NODE_EXAMPLES PROPERTIES FOLDER Wrappers/NodeJS)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pkg-fetch.success
  COMMAND ${TOOL_PATH}pkg-fetch node${NODE_VERSOIN} ${OS} ${ARCH} && touch ${CMAKE_CURRENT_BINARY_DIR}/pkg-fetch.success
  DEPENDS BUILD_NODE_ADDON
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  COMMENT "${TOOL_PATH}pkg-fetch node${NODE_VERSOIN} ${OS} ${ARCH}"
)

# Dist examples
file(GLOB nodejs_examples "nodejs-*.js")
foreach(example ${nodejs_examples})
  get_filename_component(outfileName ${example} NAME_WE)

  if(WIN32)
    set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfileName}.exe")
    # Add "| echo" to ignore the pkg warnins on msbuild, otherwise the build stops.
    add_custom_command(OUTPUT "${outfile}"
        COMMAND ${TOOL_PATH}pkg ${example} --targets=${PKG_STR} --out-path=${CMAKE_CURRENT_BINARY_DIR} | echo
        DEPENDS BUILD_NODE_EXAMPLES ${example} ${CMAKE_CURRENT_BINARY_DIR}/pkg-fetch.success
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Compile ${example} into ${outfile}"
    )
    set(packagedExamples ${packagedExamples} "${outfile}")
  else()
    set(outfile "${CMAKE_CURRENT_BINARY_DIR}/${outfileName}")
    add_custom_command(OUTPUT "${outfile}"
        COMMAND ${TOOL_PATH}pkg ${example} --targets=${PKG_STR} --out-path=${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS BUILD_NODE_EXAMPLES ${example} ${CMAKE_CURRENT_BINARY_DIR}/pkg-fetch.success
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Compile ${example} into ${outfile}"
    )
    set(packagedExamples ${packagedExamples} "${outfile}")
  endif(WIN32)
endforeach(example)

add_custom_target(
  DIST_NODE_EXAMPLES ALL
  DEPENDS ${packagedExamples}
)
set_target_properties(DIST_NODE_EXAMPLES PROPERTIES FOLDER Wrappers/NodeJS)

add_custom_target(RealsenseNodeJSExamples DEPENDS DIST_NODE_EXAMPLES)
set_target_properties(RealsenseNodeJSExamples PROPERTIES FOLDER Wrappers/NodeJS)

# Install packaged samples and required files
install(PROGRAMS ${packagedExamples} DESTINATION ${CMAKE_INSTALL_BINDIR})
# TODO: Use buildtype instead of hardcode of Release
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../build/Release/node_librealsense.node DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-glfw-3/build/Release/glfw.node DESTINATION ${CMAKE_INSTALL_BINDIR})
