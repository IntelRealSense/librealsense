# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.6)
set(ANDROID on CACHE BOOL "ANDROID")
set(APP_PLATFORM "android-16" CACHE STRING "ANDROID")
set(BUILD_UNIT_TESTS off CACHE BOOL "BUILD_UNIT_TESTS")
set(BUILD_EXAMPLES off CACHE BOOL "BUILD_EXAMPLES")
set(FORCE_USBHOST_UVC on CACHE BOOL "FORCE_USBHOST_UVC")
set(HWM_OVER_XU on CACHE BOOL "HWM_OVER_XU")
add_subdirectory(third_party/librealsense)
add_subdirectory(src/main/cpp)


