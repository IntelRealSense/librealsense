cmake_minimum_required(VERSION 3.1)

project(RealsenseExamples)

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
    list(APPEND DEPENDENCIES m ${GLFW_LIBRARIES} ${LibUSB_LIBRARIES})
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

add_executable(cpp-capture cpp-capture.cpp)
target_link_libraries(cpp-capture ${DEPENDENCIES})

add_executable(cpp-config-ui cpp-config-ui.cpp)
target_link_libraries(cpp-config-ui ${DEPENDENCIES})

add_executable(cpp-enumerate-devices cpp-enumerate-devices.cpp)
target_link_libraries(cpp-enumerate-devices ${DEPENDENCIES})

add_executable(cpp-headless cpp-headless.cpp)
target_link_libraries(cpp-headless ${DEPENDENCIES})

add_executable(cpp-multicam cpp-multicam.cpp)
target_link_libraries(cpp-multicam ${DEPENDENCIES})

add_executable(cpp-pointcloud cpp-pointcloud.cpp)
target_link_libraries(cpp-pointcloud ${DEPENDENCIES})

add_executable(cpp-restart cpp-restart.cpp)
target_link_libraries(cpp-restart ${DEPENDENCIES})

install(
    TARGETS 
    c-tutorial-1-depth 
    c-tutorial-2-streams
    c-tutorial-3-pointcloud
    
    cpp-tutorial-1-depth
    cpp-tutorial-2-streams
    cpp-tutorial-3-pointcloud
    
    cpp-alignimages
    cpp-capture
    cpp-config-ui
    cpp-enumerate-devices
    cpp-headless
    cpp-multicam
    cpp-pointcloud
    cpp-restart
    
    RUNTIME DESTINATION 
    ${CMAKE_INSTALL_PREFIX}/bin
)


