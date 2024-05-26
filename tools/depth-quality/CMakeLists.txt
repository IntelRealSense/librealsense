# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019-2024 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.8)

project( rs-depth-quality )

if(WIN32)
    add_executable( ${PROJECT_NAME} WIN32 "" )
else()
    add_executable( ${PROJECT_NAME} "" )
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_BINARY_DIR})

using_easyloggingpp( ${PROJECT_NAME} SHARED )

include(../../common/CMakeLists.txt)

SET(DELAYED
    realsense2d.dll
    realsense2-gld.dll
    realsense2.dll
    realsense2-gl.dll
)

if(BUILD_GRAPHICAL_EXAMPLES)
    set(RS_QUALITY_TOOL_CPP
        ${COMMON_SRC}
        rs-depth-quality.cpp
        depth-quality-model.h
        depth-quality-model.cpp
        depth-metrics.h
        ../../common/realsense-ui-advanced-mode.h
        ../../third-party/imgui/imgui.cpp
        ../../third-party/imgui/imgui_draw.cpp
        ../../third-party/imgui/imgui_impl_glfw.cpp
        ../../third-party/imgui/imgui-fonts-karla.hpp
        ../../third-party/imgui/imgui-fonts-fontawesome.hpp
        ../../third-party/glad/glad.c
        ../../third-party/tinyfiledialogs/tinyfiledialogs.c
        ../../third-party/tinyfiledialogs/tinyfiledialogs.h
        )

    if(WIN32)
        target_sources( ${PROJECT_NAME}
            PRIVATE
                ${RS_QUALITY_TOOL_CPP}
                ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc
                ../../common/windows-app-bootstrap.cpp
            )

        set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
        source_group("Resources" FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc)

        source_group("3rd Party" FILES
            ../../third-party/tinyfiledialogs/tinyfiledialogs.c
            ../../third-party/tinyfiledialogs/tinyfiledialogs.h
            ../../third-party/imgui/imgui.cpp
            ../../third-party/imgui/imgui_draw.cpp
            ../../third-party/imgui/imgui_impl_glfw.cpp
            ../../third-party/imgui/imgui-fonts-karla.hpp
            ../../third-party/imgui/imgui-fonts-fontawesome.hpp)

        source_group("Utils" FILES
            depth-quality-model.h
            depth-quality-model.cpp
            depth-metrics.h
            ${COMMON_SRC})

        include_directories(../../common  ../../third-party
                            ../../third-party/imgui
                            ../../third-party/glad
                            ../../third-party/tinyfiledialogs
                            ${CMAKE_CURRENT_SOURCE_DIR}/res/)

        #list(TRANSFORM DELAYED PREPEND " /DELAYLOAD:")
        PREPEND(DELAYED " /DELAYLOAD:" ${DELAYED})
        #infoValue(DELAYED)
        string(REPLACE ";" " " LD_FLAGS_STR "${DELAYED}")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${LD_FLAGS_STR}")

    else()
        target_sources( ${PROJECT_NAME}
            PRIVATE
                ${RS_QUALITY_TOOL_CPP}
            )

        include_directories(../../common  ../../third-party
                            ../../third-party/imgui
                            ../../third-party/glad
                            ../../third-party/tinyfiledialogs )
    endif()

    source_group("SW-Update" FILES ${SW_UPDATE_FILES})

    if (CHECK_FOR_UPDATES)
        message( STATUS "Check for updates capability added to realsense-depth-quality" )
        add_dependencies(${PROJECT_NAME} libcurl)
        target_link_libraries(${PROJECT_NAME} curl)
    endif()

    target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES}
                                           ${GTK3_LIBRARIES}
                                           Threads::Threads
                                           realsense2-gl
                                           tclap )

    if (IMPORT_DEPTH_CAM_FW)
        add_definitions(-DINTERNAL_FW)
        target_link_libraries(${PROJECT_NAME} fw)
    endif()




    set_target_properties (${PROJECT_NAME} PROPERTIES
        FOLDER Tools
    )

target_include_directories(${PROJECT_NAME} PRIVATE ../../src)

    install(
        TARGETS

        ${PROJECT_NAME}

        RUNTIME DESTINATION
        ${CMAKE_INSTALL_BINDIR}
    )
endif()
