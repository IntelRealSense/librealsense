#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseToolsDepthQuality)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

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

if(BUILD_GRAPHICAL_EXAMPLES)
    set(RS_QUALITY_TOOL_CPP
            ./rs-depth-quality.cpp
            ./depth-quality-model.h
            ./depth-quality-model.cpp
            ./depth-metrics.h
            ../../common/rendering.h
            ../../common/ux-window.h
            ../../common/ux-window.cpp
            ../../common/model-views.h
            ../../common/model-views.cpp
            ../../third-party/imgui/imgui.cpp
            ../../third-party/imgui/imgui_draw.cpp
            ../../third-party/imgui/imgui_impl_glfw.cpp
            ../../third-party/imgui/imgui-fonts-karla.hpp
            ../../third-party/imgui/imgui-fonts-fontawesome.hpp
        )

    if(WIN32)
        add_executable(rs-depth-quality WIN32 ${RS_QUALITY_TOOL_CPP}
                ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.h
                ${CMAKE_CURRENT_SOURCE_DIR}/res/realsense-viewer.rc
                ../../common/windows-app-bootstrap.cpp)

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
                                             ../../third-party/tclap/include
                                             ${CMAKE_CURRENT_SOURCE_DIR}/res/)

    else()
        add_executable(rs-depth-quality ${RS_QUALITY_TOOL_CPP})

        include_directories(rs-depth-quality ../../common  ../../third-party
                                             ../../third-party/imgui
                                             ../../third-party/tclap/include)
    endif()

    target_link_libraries(rs-depth-quality ${DEPENDENCIES} ${GTK3_LIBRARIES})
    set_target_properties (rs-depth-quality PROPERTIES
        FOLDER Tools
    )

    install(
        TARGETS

        rs-depth-quality

        RUNTIME DESTINATION
        ${CMAKE_INSTALL_PREFIX}/bin
    )
endif()
