# ubuntu 12.04 LTS cmake version 2.8.7
# ubuntu 14.04 LTS cmake version 2.8.12.2
# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

project(RealsenseExamples)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

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


add_subdirectory(capture)
add_subdirectory(save-to-disk)
add_subdirectory(multicam)
add_subdirectory(pointcloud)
add_subdirectory(align)