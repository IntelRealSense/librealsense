# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.6)

include("../config.cmake")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LRS_ANDROID_ROOT_DIR}/build/jniLibs/${ANDROID_ABI})
add_subdirectory(${LRS_ROOT_DIR} ${LRS_ANDROID_ROOT_DIR}/build)
#include_directories(${LRS_ROOT_DIR}/include)