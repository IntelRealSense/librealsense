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
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

find_package(OpenGL REQUIRED)
set(DEPENDENCIES realsense ${OPENGL_LIBRARIES})

if(WIN32)
    add_subdirectory(third_party/glfw)
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

# C Tutorials
add_executable(c-tutorial-1-depth c-tutorial-1-depth.c)
target_link_libraries(c-tutorial-1-depth ${DEPENDENCIES})

add_executable(c-tutorial-2-streams c-tutorial-2-streams.c)
target_link_libraries(c-tutorial-2-streams ${DEPENDENCIES})

add_executable(c-tutorial-3-pointcloud c-tutorial-3-pointcloud.c)
target_link_libraries(c-tutorial-3-pointcloud ${DEPENDENCIES})

# C++ Tutorials
add_executable(cpp-tutorial-1-depth cpp-tutorial-1-depth.cpp)
target_link_libraries(cpp-tutorial-1-depth ${DEPENDENCIES})

add_executable(cpp-tutorial-2-streams cpp-tutorial-2-streams.cpp)
target_link_libraries(cpp-tutorial-2-streams ${DEPENDENCIES})

add_executable(cpp-tutorial-3-pointcloud cpp-tutorial-3-pointcloud.cpp)
target_link_libraries(cpp-tutorial-3-pointcloud ${DEPENDENCIES})

# Examples
add_executable(cpp-alignimages cpp-alignimages.cpp)
target_link_libraries(cpp-alignimages ${DEPENDENCIES})

add_executable(cpp-callback cpp-callback.cpp)
target_link_libraries(cpp-callback ${DEPENDENCIES})

add_executable(cpp-callback-2 cpp-callback-2.cpp)
target_link_libraries(cpp-callback-2 ${DEPENDENCIES})

add_executable(cpp-capture cpp-capture.cpp)
target_link_libraries(cpp-capture ${DEPENDENCIES})

add_executable(cpp-config-ui cpp-config-ui.cpp)
target_link_libraries(cpp-config-ui ${DEPENDENCIES})

add_executable(cpp-enumerate-devices cpp-enumerate-devices.cpp)
target_link_libraries(cpp-enumerate-devices ${DEPENDENCIES})

add_executable(cpp-headless cpp-headless.cpp)
target_link_libraries(cpp-headless ${DEPENDENCIES})

add_executable(cpp-motion-module cpp-motion-module.cpp)
target_link_libraries(cpp-motion-module ${DEPENDENCIES})

add_executable(cpp-multicam cpp-multicam.cpp)
target_link_libraries(cpp-multicam ${DEPENDENCIES})

add_executable(cpp-pointcloud cpp-pointcloud.cpp)
target_link_libraries(cpp-pointcloud ${DEPENDENCIES})

add_executable(cpp-restart cpp-restart.cpp)
target_link_libraries(cpp-restart ${DEPENDENCIES})

add_executable(cpp-stride cpp-stride.cpp)
target_link_libraries(cpp-stride ${DEPENDENCIES})

install(
    TARGETS
    c-tutorial-1-depth
    c-tutorial-2-streams
    c-tutorial-3-pointcloud

    cpp-tutorial-1-depth
    cpp-tutorial-2-streams
    cpp-tutorial-3-pointcloud

    cpp-alignimages
    cpp-callback
    cpp-callback-2
    cpp-capture
    cpp-config-ui
    cpp-enumerate-devices
    cpp-headless
    cpp-motion-module
    cpp-multicam
    cpp-pointcloud
    cpp-restart
    cpp-stride

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_BINDIR}
)


