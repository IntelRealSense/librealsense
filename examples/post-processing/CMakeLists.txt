#  minimum required cmake version: 3.8.0
cmake_minimum_required(VERSION 3.8.0)

project(RealsenseExamplesPost-Processing)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

if(BUILD_GRAPHICAL_EXAMPLES)
    add_executable(rs-post-processing rs-post-processing.cpp ../example.hpp ../../third-party/imgui/imgui.cpp ../../third-party/imgui/imgui_draw.cpp ../../third-party/imgui/imgui_impl_glfw.cpp)
    target_link_libraries(rs-post-processing ${DEPENDENCIES})
    include_directories(rs-post-processing ../ ../../third-party/tclap/include ../../third-party/imgui)
    set_target_properties (rs-post-processing PROPERTIES
        FOLDER "Examples"
    )

    install(
        TARGETS

        rs-post-processing

        RUNTIME DESTINATION
        ${CMAKE_INSTALL_PREFIX}/bin
    )
endif()
