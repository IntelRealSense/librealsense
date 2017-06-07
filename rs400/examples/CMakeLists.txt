cmake_minimum_required(VERSION 2.8.3)

project(RS400_Advanced_Mode_Samples)

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

add_executable(rs400-advanced-mode-sample sample.cpp ../include/r400_advanced_mode/r400_advanced_mode.hpp third_party/json.hpp json_loader.hpp
    ../../examples/third_party/imgui/imgui.cpp ../../examples/third_party/imgui/imgui_draw.cpp ../../examples/third_party/imgui/imgui_impl_glfw.cpp sample.hpp ../include/r400_advanced_mode/AdvancedModeCommand.h)
target_link_libraries(rs400-advanced-mode-sample ${DEPENDENCIES})
include_directories(rs400-advanced-mode-sample ../../examples/third_party/imgui third_party ../../examples/third_party/tclap/include)

file(GLOB JSONS
  "presets/*.json"
)
file(COPY ${JSONS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/presets)

set_target_properties (rs400-advanced-mode-sample PROPERTIES
    FOLDER "RS400 Extras"
)

if(WIN32)
    source_group("Header Files\\3rd Party" FILES
        third_party/json.hpp)

    source_group("Source Files\\3rd Party" FILES
        ../../examples/third_party/imgui/imgui.cpp 
        ../../examples/third_party/imgui/imgui_impl_glfw.cpp)

    source_group("Header Files\\Reference" FILES
        ../include/r400_advanced_mode/AdvancedModeCommand.h)

endif()

install(
    TARGETS
    rs400-advanced-mode-sample
    
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
)

if(WIN32)
    set(ADVANCED_DEF advanced-c.def)
endif()
add_library(rs400-advanced-mode SHARED advanced-c.h advanced-c.cpp ../include/r400_advanced_mode/AdvancedModeCommand.h ${ADVANCED_DEF} c-sample.c)
target_link_libraries(rs400-advanced-mode ${DEPENDENCIES})
include_directories(rs400-advanced-mode ../include/r400_advanced_mode ../../include)

set_target_properties (rs400-advanced-mode PROPERTIES
    FOLDER "RS400 Extras"
)

