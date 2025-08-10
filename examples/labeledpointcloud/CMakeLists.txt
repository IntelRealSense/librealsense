# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project(RealsenseExamplesLabeledPointcloud)

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-labeled-pointcloud rs-labeled-pointcloud.cpp ../example.hpp labeled-pointcloud-drawing.hpp ../../common/labeled-point-cloud-utilities.cpp ${IMGUI_SOURCES})
    set_property(TARGET rs-labeled-pointcloud PROPERTY CXX_STANDARD 11)
    target_link_libraries(rs-labeled-pointcloud ${DEPENDENCIES})
    include_directories(../)
    set_target_properties (rs-labeled-pointcloud PROPERTIES FOLDER Examples)
    install(TARGETS rs-labeled-pointcloud RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
