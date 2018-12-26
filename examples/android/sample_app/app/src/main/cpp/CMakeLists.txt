cmake_minimum_required(VERSION 3.4.1)

project(android_librs_test)
find_library( # Sets the name of the path variable.
        log-lib
        # Specifies the name of the NDK library that you want CMake to locate.
        log)
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h src/*.c)
file(GLOB_RECURSE HEADERS include/*.h)
# shared lib will also be tucked into APK and sent to target
# refer to app/build.gradle, jniLibs section for that purpose.
# ${ANDROID_ABI} is handy for our purpose here. Probably this ${ANDROID_ABI} is
# the most valuable thing of this sample, the rest are pretty much normal cmake
set(librs_bin_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../librealsense_module/src/main/jniLibs)
add_library(realsense2 SHARED IMPORTED)
set_target_properties(realsense2 PROPERTIES IMPORTED_LOCATION
        ${librs_bin_DIR}/librealsense2.so)

add_library(
        android_librs_test
        SHARED
        ${SOURCES}
        ${HEADERS})

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.


target_link_libraries( # Specifies the target library.
        android_librs_test

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        realsense2
        )
# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(
        android_librs_test
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE src)
