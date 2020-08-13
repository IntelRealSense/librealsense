# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsDepthQuality)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_BINARY_DIR})

set( ELPP_DIR ${CMAKE_SOURCE_DIR}/third-party/easyloggingpp/src )

include_directories( ${ELPP_DIR} )
set( ELPP_FILES
    ${ELPP_DIR}/easylogging++.cc
    ${ELPP_DIR}/easylogging++.h)
    
include(${CMAKE_SOURCE_DIR}/common/CMakeLists.txt)

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
        ${CMAKE_SOURCE_DIR}/common/realsense-ui-advanced-mode.h
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui.cpp
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_draw.cpp
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui-fonts-karla.hpp
        ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui-fonts-fontawesome.hpp
        ${CMAKE_SOURCE_DIR}/third-party/glad/glad.c
        ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs/tinyfiledialogs.c
        ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs/tinyfiledialogs.h
        )

    if(WIN32)
        add_executable(rs-depth-quality WIN32 ${RS_QUALITY_TOOL_CPP}
                ${ELPP_FILES}
                ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc
                ${CMAKE_SOURCE_DIR}/common/windows-app-bootstrap.cpp)

        set_property(TARGET rs-depth-quality PROPERTY CXX_STANDARD 11)
        source_group("Resources" FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc)

        source_group("3rd Party" FILES
            ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs/tinyfiledialogs.c
            ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs/tinyfiledialogs.h
            ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui.cpp
            ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_draw.cpp
            ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_impl_glfw.cpp
            ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui-fonts-karla.hpp
            ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui-fonts-fontawesome.hpp)

        source_group("Utils" FILES
            depth-quality-model.h
            depth-quality-model.cpp
            depth-metrics.h
            ${COMMON_SRC})

        include_directories(rs-depth-quality ${CMAKE_SOURCE_DIR}/common  ${CMAKE_SOURCE_DIR}/third-party
                                             ${CMAKE_SOURCE_DIR}/third-party/imgui
                                             ${CMAKE_SOURCE_DIR}/third-party/glad
                                             ${CMAKE_SOURCE_DIR}/third-party/tclap/include
                                             ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs
                                             ${CMAKE_CURRENT_SOURCE_DIR}/res/)

        #list(TRANSFORM DELAYED PREPEND " /DELAYLOAD:")
        PREPEND(DELAYED " /DELAYLOAD:" ${DELAYED})
        #infoValue(DELAYED)
        string(REPLACE ";" " " LD_FLAGS_STR "${DELAYED}")
        set_target_properties(rs-depth-quality PROPERTIES LINK_FLAGS "${LD_FLAGS_STR}")

    else()
        add_executable(rs-depth-quality 
        ${RS_QUALITY_TOOL_CPP}
        ${ELPP_FILES}
        )

        include_directories(rs-depth-quality ${CMAKE_SOURCE_DIR}/common  ${CMAKE_SOURCE_DIR}/third-party
                                             ${CMAKE_SOURCE_DIR}/third-party/imgui
                                             ${CMAKE_SOURCE_DIR}/third-party/glad
                                             ${CMAKE_SOURCE_DIR}/third-party/tinyfiledialogs
                                             ${CMAKE_SOURCE_DIR}/third-party/tclap/include)
    endif()

    source_group("EasyLogging++" FILES ${ELPP_FILES})
    source_group("SW-Update" FILES ${SW_UPDATE_FILES})

    if (CHECK_FOR_UPDATES)
        message( STATUS "Check for updates capability added to realsense-depth-quality" )
        add_dependencies(rs-depth-quality libcurl)
        target_link_libraries(rs-depth-quality curl)
    endif()
    
    target_link_libraries(rs-depth-quality ${DEPENDENCIES}
                                           ${GTK3_LIBRARIES}
                                           Threads::Threads
                                           realsense2-gl)

    if (IMPORT_DEPTH_CAM_FW)
        add_definitions(-DINTERNAL_FW)
        target_link_libraries(rs-depth-quality fw)
    endif()
    
    


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
