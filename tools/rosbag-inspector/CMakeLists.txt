# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

project(RealsenseToolsRosbagInspector)

if(BUILD_GRAPHICAL_EXAMPLES)

    if(APPLE)
        add_definitions(-DNOC_FILE_DIALOG_OSX)
    elseif(UNIX)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

        include_directories(${GTK3_INCLUDE_DIRS})
        link_directories(${GTK3_LIBRARY_DIRS})
        add_definitions(${GTK3_CFLAGS_OTHER})

        add_definitions(-DNOC_FILE_DIALOG_GTK)
    elseif(WIN32)
        add_definitions(-DNOC_FILE_DIALOG_WIN32)
    endif()

    set(RS_ROSBAG_INSPECTOR_CPP
            rs-rosbag-inspector.cpp
            files_container.h
            print_helpers.h
            rosbag_content.h
            ../../third-party/imgui/imgui.cpp
            ../../third-party/imgui/imgui_draw.cpp
            ../../third-party/imgui/imgui_impl_glfw.cpp
            ../../third-party/imgui/imgui-fonts-karla.hpp
            ../../third-party/imgui/imgui-fonts-fontawesome.hpp
        )

    if(WIN32)
        add_executable(rs-rosbag-inspector WIN32 ${RS_ROSBAG_INSPECTOR_CPP})

        include_directories(rs-rosbag-inspector ../../third-party/imgui ../../common ../../third-party ${CMAKE_CURRENT_SOURCE_DIR}/res/)
    else()
        add_executable(rs-rosbag-inspector ${RS_ROSBAG_INSPECTOR_CPP})
        include_directories(rs-rosbag-inspector ../../third-party/imgui ../../common ../../third-party)
    endif()
    set_property(TARGET rs-rosbag-inspector PROPERTY CXX_STANDARD 11)

    target_include_directories(rs-rosbag-inspector PRIVATE
        ${ROSBAG_HEADER_DIRS}
        ${BOOST_INCLUDE_PATH}
        ${LZ4_INCLUDE_PATH}
        )

    target_link_libraries(rs-rosbag-inspector realsense-file ${DEPENDENCIES} ${GTK3_LIBRARIES})

    set_target_properties (rs-rosbag-inspector PROPERTIES
        FOLDER Tools
    )

    install(
        TARGETS

        rs-rosbag-inspector

        RUNTIME DESTINATION
        ${CMAKE_INSTALL_BINDIR}
    )
endif()
