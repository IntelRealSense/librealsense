#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)
project(librealsense2 LANGUAGES CXX C)

# include librealsense general configuration
include(CMake/global_config.cmake)


config_cxx_flags()

# include os specific macros
# macro definition located at "CMake/global_config.cmake"
include(CMake/include_os.cmake)

# set os specific configuration flags
# macro definition located at "CMake/<OS>_config.cmake"
os_set_flags()

# set global configuration flags
# macro definition located at "CMake/global_config.cmake"
global_set_flags()

if(BUILD_SHARED_LIBS)
    add_library(${LRS_TARGET} SHARED "")
else()
    add_library(${LRS_TARGET} STATIC "")
endif()

# set library version
set_target_properties(${LRS_TARGET} PROPERTIES VERSION ${REALSENSE_VERSION_STRING} SOVERSION "${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}")

# Checking Internet connection, as TM2 needs to download the FW from amazon cloud
if(BUILD_WITH_TM2)
    include(CMake/tm2_connectivity_check.cmake)
endif()

include(include/CMakeLists.txt)
include(src/CMakeLists.txt)
include(third-party/CMakeLists.txt)

# configure the project according to OS specific requirments
# macro definition located at "CMake/<OS>_config.cmake"
os_target_config()

# global project configuration
# macro definition located at "CMake/global_config.cmake"
global_target_config()
option(BUILD_CV_KINFU_EXAMPLE "Build OpenCV KinectFusion example" OFF)

add_subdirectory(wrappers)

if (BUILD_EXAMPLES AND BUILD_GRAPHICAL_EXAMPLES)
    add_subdirectory(third-party/glfw)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
    add_subdirectory(tools)
endif()

if(BUILD_UNIT_TESTS)
    add_subdirectory(unit-tests)
endif()

if (BUILD_WITH_TM2)
    add_tm2()
endif()

include(CMake/install_config.cmake)
