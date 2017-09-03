cmake_minimum_required(VERSION 2.8.3)

project(RealsenseToolsRosbagInspector)

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

# terminal
add_executable(rs-rosbag-inspector rs-rosbag-inspector.cpp)
if(WIN32)
    target_link_libraries(rs-rosbag-inspector ${DEPENDENCIES})
else()
    target_link_libraries(rs-rosbag-inspector -lpthread ${DEPENDENCIES})
endif()

include_directories(rs-rosbag-inspector 
				    ../../common 
					../../third-party)

set_target_properties (
	rs-rosbag-inspector 
	PROPERTIES
    FOLDER Tools
)

install(
    TARGETS
    rs-rosbag-inspector
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
)

