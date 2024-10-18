# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project( rs-align-gl )

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(BUILD_GRAPHICAL_EXAMPLES AND NOT APPLE)

add_executable( ${PROJECT_NAME} rs-align-gl.cpp
    ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp
    )
set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11 )
target_link_libraries( ${PROJECT_NAME} ${DEPENDENCIES} realsense2-gl )
include_directories( ../../third-party/imgui ../../examples )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Examples )
using_easyloggingpp( ${PROJECT_NAME} SHARED )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

endif()
