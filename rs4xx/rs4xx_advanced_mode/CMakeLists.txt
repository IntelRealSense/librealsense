cmake_minimum_required(VERSION 2.8.3)

project(RS4XX_Advanced_Mode)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

find_package(OpenGL REQUIRED)
set(DEPENDENCIES ${OPENGL_LIBRARIES} realsense2)

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

set(RS4XX_SOURCES
    src/advanced_mode.cpp
    src/presets.h
    src/presets.cpp
    src/rs2_advanced_mode.cpp
    src/json_loader.hpp
    )

set(RS4XX_INCLUDES
    ../../include/librealsense/rs2_advanced_mode.h
    ../../include/librealsense/rs2_advanced_mode.hpp
    ../../include/librealsense/rs2_advanced_mode_command.h
    )

if(WIN32)
    source_group("Header Files\\3rd Party" FILES
        ../../third-party/json.hpp)

    source_group("Source Files\\3rd Party" FILES
        ../../third-party/imgui/imgui.cpp
        ../../third-party/imgui/imgui_impl_glfw.cpp)

    source_group("Header Files\\Reference" FILES
        ../../include/librealsense/rs2_advanced_mode_command.h)

endif()

add_library(rs2-advanced-mode SHARED ${RS4XX_SOURCES} ${RS4XX_INCLUDES} ${ADVANCED_DEF})
target_link_libraries(rs2-advanced-mode ${DEPENDENCIES})
include_directories(rs2-advanced-mode include)

set_target_properties (rs2-advanced-mode PROPERTIES
    FOLDER "RS4XX Advanced Mode"
)

file(GLOB JSONS
  "./presets/*.json"
)
file(COPY ${JSONS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/presets)
