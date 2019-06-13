# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsRealSenseViewer)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

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

    set(RS_VIEWER_CPP
            realsense-viewer.cpp
            ../../third-party/imgui/imgui.cpp
            ../../third-party/imgui/imgui_draw.cpp
            ../../third-party/imgui/imgui_impl_glfw.cpp
            ../../third-party/imgui/imgui-fonts-karla.hpp
            ../../third-party/imgui/imgui-fonts-fontawesome.hpp
            ../../common/rendering.h
            ../../common/realsense-ui-advanced-mode.h
            ../../common/model-views.h
            ../../common/model-views.cpp
            ../../common/ux-window.h
            ../../common/ux-window.cpp
            ../../common/ux-alignment.cpp
            ../../common/ux-alignment.h
            ../../third-party/glad/glad.c
            ../../common/opengl3.cpp
            ../../common/opengl3.h
            ../../common/rs-config.h
            ../../common/rs-config.cpp
            ../../common/os.h
            ../../common/os.cpp
        )

    # config-ui
    if(WIN32)
        add_executable(realsense-viewer WIN32
                ${RS_VIEWER_CPP} ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc
                ../../common/windows-app-bootstrap.cpp)

        source_group("3rd Party" FILES
            ../../third-party/imgui/imgui.cpp
            ../../third-party/imgui/imgui_draw.cpp
            ../../third-party/imgui/imgui_impl_glfw.cpp
            ../../third-party/imgui/imgui-fonts-karla.hpp
            ../../third-party/imgui/imgui-fonts-fontawesome.hpp
            )

        source_group("Resources" FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc)

        include_directories(realsense-viewer ../../third-party/imgui 
        ../../third-party/glad ../../common
        ../../third-party ${CMAKE_CURRENT_SOURCE_DIR}/res/)
    else()
        add_executable(realsense-viewer ${RS_VIEWER_CPP})
        include_directories(realsense-viewer ../../third-party/imgui 
        ../../third-party/glad ../../common ../../third-party)
    endif()
    set_property(TARGET realsense-viewer PROPERTY CXX_STANDARD 11)

    target_link_libraries(realsense-viewer ${DEPENDENCIES}
                                           ${GTK3_LIBRARIES}
                                           Threads::Threads
                                           realsense2-gl
                                           )

    set_target_properties (realsense-viewer PROPERTIES
        FOLDER Tools
    )

    install(
        TARGETS

        realsense-viewer

        RUNTIME DESTINATION
        ${CMAKE_INSTALL_BINDIR}
    )
endif()
