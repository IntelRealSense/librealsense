# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.6)
if(DEFINED ENV{LIBREALSENSE_SOURCE_ROOT_DIR})    
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/main/jniLibs)
    add_subdirectory($ENV{LIBREALSENSE_SOURCE_ROOT_DIR} ${CMAKE_CURRENT_BINARY_DIR}/librealsense )
else()
    message(FATAL_ERROR "Please set the LIBREALSENSE_SOURCE_ROOT_DIR system enviroment variable to Librealsense source root directory")
endif()
