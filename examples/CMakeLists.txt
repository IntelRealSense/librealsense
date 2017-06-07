# ubuntu 12.04 LTS cmake version 2.8.7
# ubuntu 14.04 LTS cmake version 2.8.12.2
# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

project(RealsenseExamples)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

# This parameter is meant for disabling graphical examples when building for
# headless targets.
option(BUILD_GRAPHICAL_EXAMPLES "Build graphical examples." ON)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

if(ANDROID)
    set(DEPENDENCIES realsense2)

    add_executable(rs-enumerate-devices cpp-enumerate-devices.cpp)
    target_link_libraries(rs-enumerate-devices ${DEPENDENCIES})
    include_directories(rs-enumerate-devices third_party/tclap/include)

    set_target_properties (rs-enumerate-devices PROPERTIES
        FOLDER Examples
    )
else()
    if(BUILD_GRAPHICAL_EXAMPLES)
        find_package(OpenGL REQUIRED)
        set(DEPENDENCIES realsense2 ${OPENGL_LIBRARIES})

        if(WIN32)
            list(APPEND DEPENDENCIES glfw3)
        else()
            # Find glfw header
            find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h
                PATHS /usr/X11R6/include
                      /usr/include/X11
                      /opt/graphics/OpenGL/include
                      /opt/graphics/OpenGL/contrib/libglfw
                      /usr/local/include
                      /usr/include/GL
                      /usr/include
            )
            # Find glfw library
            find_library(GLFW_LIBRARIES NAMES glfw glfw3
                    PATHS /usr/lib64
                          /usr/lib
                          /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                          /usr/local/lib64
                          /usr/local/lib
                          /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
                          /usr/X11R6/lib
            )
            list(APPEND DEPENDENCIES m ${GLFW_LIBRARIES} ${LIBUSB1_LIBRARIES})
            include_directories(${GLFW_INCLUDE_DIR})
        endif()
    else()
        set(DEPENDENCIES realsense2)
        if(NOT WIN32)
            list(APPEND DEPENDENCIES m ${LIBUSB1_LIBRARIES})
        endif()
    endif()


    # C/C++ Headless Tutorials

    # c-tutorial-1-depth
    add_executable(c-tutorial-1-depth c-tutorial-1-depth.c)
    target_link_libraries(c-tutorial-1-depth ${DEPENDENCIES})
    set_target_properties (c-tutorial-1-depth PROPERTIES
        FOLDER "C Tutorials"
    )

    # cpp-tutorial-1-depth
    add_executable(cpp-tutorial-1-depth cpp-tutorial-1-depth.cpp)
    target_link_libraries(cpp-tutorial-1-depth ${DEPENDENCIES})
    set_target_properties (cpp-tutorial-1-depth PROPERTIES
        FOLDER "C++ Tutorials"
    )

    # data-collect
    add_executable(rs-data-collect cpp-data-collect.cpp)
    target_link_libraries(rs-data-collect ${DEPENDENCIES})
    include_directories(rs-data-collect third_party/tclap/include)
    set_target_properties (rs-data-collect PROPERTIES
        FOLDER "Examples"
    )

    # headless
    add_executable(rs-headless cpp-headless.cpp)
    target_link_libraries(rs-headless ${DEPENDENCIES})
    include_directories(rs-headless third_party/tclap/include)
    set_target_properties (rs-headless PROPERTIES
        FOLDER "Examples"
    )

    # enumerate-devices
    add_executable(rs-enumerate-devices cpp-enumerate-devices.cpp)
    target_link_libraries(rs-enumerate-devices ${DEPENDENCIES})
    include_directories(rs-enumerate-devices third_party/tclap/include)
    set_target_properties (rs-enumerate-devices PROPERTIES
        FOLDER Examples
    )

    # terminal
    add_executable(rs-terminal cpp-terminal.cpp cpp-terminal-helpers/auto-complete/auto-complete.cpp cpp-terminal-helpers/auto-complete/auto-complete.h)
    if(WIN32)
        target_link_libraries(rs-terminal ${DEPENDENCIES})
    else()
        target_link_libraries(rs-terminal -lpthread ${DEPENDENCIES})
    endif()
    include_directories(rs-terminal third_party/tclap/include cpp-terminal-helpers/auto-complete)
    set_target_properties (rs-terminal PROPERTIES
        FOLDER Examples
    )

    # fw-logger
    add_executable(rs-fw-logger cpp-fw-logger.cpp cpp-fw-logger-helpers/fw-log-data.cpp
        cpp-fw-logger-helpers/fw-log-data.h
        cpp-fw-logger-helpers/fw-logs-formating-options.cpp
        cpp-fw-logger-helpers/fw-logs-formating-options.h
        cpp-fw-logger-helpers/fw-logs-parser.cpp
        cpp-fw-logger-helpers/fw-logs-parser.h
        cpp-fw-logger-helpers/fw-logs-xml-helper.cpp
        cpp-fw-logger-helpers/fw-logs-xml-helper.h
        cpp-fw-logger-helpers/string-formatter.cpp
        cpp-fw-logger-helpers/string-formatter.h)
    if(WIN32)
        target_link_libraries(rs-fw-logger ${DEPENDENCIES})
    else()
        target_link_libraries(rs-fw-logger -lpthread ${DEPENDENCIES})
    endif()
    include_directories(rs-fw-logger third_party/tclap/include cpp-fw-logger-helpers)
    set_target_properties (rs-fw-logger PROPERTIES
        FOLDER Examples
    )

    install(
        TARGETS

        rs-enumerate-devices
        rs-terminal
        rs-fw-logger
        rs-data-collect
        rs-headless

        RUNTIME DESTINATION
        ${CMAKE_INSTALL_PREFIX}/bin
    )

    if(BUILD_GRAPHICAL_EXAMPLES)
        # C/C++ Tutorials

        # c-tutorial-2-streams
        add_executable(c-tutorial-2-streams c-tutorial-2-streams.c)
        target_link_libraries(c-tutorial-2-streams ${DEPENDENCIES})
        set_target_properties (c-tutorial-2-streams PROPERTIES
            FOLDER "C Tutorials"
        )

        # config-ui
        add_executable(rs-config-ui cpp-config-ui.cpp third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp third_party/imgui/imgui_impl_glfw.cpp example.hpp)
        target_link_libraries(rs-config-ui ${DEPENDENCIES})
        include_directories(rs-config-ui third_party/imgui)
        set_target_properties (rs-config-ui PROPERTIES
            FOLDER Examples
        )

        # capture
        add_executable(rs-capture cpp-capture.cpp)
        target_link_libraries(rs-capture ${DEPENDENCIES})
        include_directories(rs-capture third_party/tclap/include)
        set_target_properties (rs-capture PROPERTIES
            FOLDER "Examples"
        )

        # tutorial-2-streams
        add_executable(cpp-tutorial-2-streams cpp-tutorial-2-streams.cpp)
        target_link_libraries(cpp-tutorial-2-streams ${DEPENDENCIES})
        set_target_properties (cpp-tutorial-2-streams PROPERTIES
            FOLDER "C++ Tutorials"
        )

        # multicam
        add_executable(rs-multicam cpp-multicam.cpp)
        target_link_libraries(rs-multicam ${DEPENDENCIES})
        set_target_properties (rs-multicam PROPERTIES
            FOLDER Examples
        )

        # pointcloud
        add_executable(rs-pointcloud cpp-pointcloud.cpp third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp third_party/imgui/imgui_impl_glfw.cpp)
        target_link_libraries(rs-pointcloud ${DEPENDENCIES})
        include_directories(rs-config-ui third_party/imgui)
        set_target_properties (rs-pointcloud PROPERTIES
            FOLDER Examples
        )

        install(
            TARGETS

            rs-config-ui
            rs-capture
            rs-multicam
            rs-pointcloud

            RUNTIME DESTINATION
            ${CMAKE_INSTALL_PREFIX}/bin
        )
    endif()
endif()

