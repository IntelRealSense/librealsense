# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsDepthQuality)

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

    set(RS_QUALITY_TOOL_CPP
            ./rs-depth-quality.cpp
            ./depth-quality-model.h
            ./depth-quality-model.cpp
            ./depth-metrics.h
            ../../common/rendering.h
            ../../common/realsense-ui-advanced-mode.h
            ../../common/ux-window.h
            ../../common/ux-window.cpp
            ../../common/model-views.h
            ../../common/model-views.cpp
            ../../third-party/imgui/imgui.cpp
            ../../third-party/imgui/imgui_draw.cpp
            ../../third-party/imgui/imgui_impl_glfw.cpp
            ../../third-party/imgui/imgui-fonts-karla.hpp
            ../../third-party/imgui/imgui-fonts-fontawesome.hpp
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

    if(WIN32)
        add_executable(rs-depth-quality WIN32 ${RS_QUALITY_TOOL_CPP}
                ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc
                ../../common/windows-app-bootstrap.cpp)

        set_property(TARGET rs-depth-quality PROPERTY CXX_STANDARD 11)
        source_group("Resources" FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc)

        source_group("3rd Party" FILES
            ../../third-party/imgui/imgui.cpp
            ../../third-party/imgui/imgui_draw.cpp
            ../../third-party/imgui/imgui_impl_glfw.cpp
            ../../third-party/imgui/imgui-fonts-karla.hpp
            ../../third-party/imgui/imgui-fonts-fontawesome.hpp)

        source_group("Utils" FILES
            ./depth-quality-model.h
            ./depth-quality-model.cpp
            ./depth-metrics.h
            ../../common/rendering.h
            ../../common/ux-window.h
            ../../common/ux-window.cpp
            ../../common/model-views.h
            ../../common/model-views.cpp
            ../../common/windows-app-bootstrap.cpp)

        include_directories(rs-depth-quality ../../common  ../../third-party
                                             ../../third-party/imgui
                                             ../../third-party/glad
                                             ../../third-party/tclap/include
                                             ${CMAKE_CURRENT_SOURCE_DIR}/res/)

    else()
        add_executable(rs-depth-quality ${RS_QUALITY_TOOL_CPP})

        include_directories(rs-depth-quality ../../common  ../../third-party
                                             ../../third-party/imgui
                                             ../../third-party/glad
                                             ../../third-party/tclap/include)
    endif()

    target_link_libraries(rs-depth-quality ${DEPENDENCIES}
                                           ${GTK3_LIBRARIES}
                                           Threads::Threads
                                           realsense2-gl)
    set_target_properties (rs-depth-quality PROPERTIES
        FOLDER Tools
    )

    install(
        TARGETS

        rs-depth-quality

        RUNTIME DESTINATION
        ${CMAKE_INSTALL_BINDIR}
    )
endif()
