# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.6)
set(HWM_OVER_XU on CACHE BOOL "HWM_OVER_XU")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../../../librealsense ${CMAKE_CURRENT_BINARY_DIR}/librealsense )

add_subdirectory(src/main/cpp)