# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

if(BUILD_WITH_DDS)
    project(Realsensetoolsdds-sniffer)

    add_executable(dds-sniffer dds-sniffer.cpp)

    set_property(TARGET dds-sniffer PROPERTY CXX_STANDARD 11)

	add_dependencies(dds-sniffer dds)
	target_link_libraries(dds-sniffer dds)
	
	set_target_properties (dds-sniffer PROPERTIES FOLDER Tools)

endif()
