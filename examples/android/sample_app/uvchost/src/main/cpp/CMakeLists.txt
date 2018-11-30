cmake_minimum_required(VERSION 3.4.1)

project(usbhost)
find_library( # Sets the name of the path variable.
        log-lib
        # Specifies the name of the NDK library that you want CMake to locate.
        log)
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h src/*.c)
file(GLOB_RECURSE HEADERS include/*.h)
add_library(
        usbhost
        SHARED
        ${SOURCES}
        ${HEADERS})

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.


target_link_libraries( # Specifies the target library.
        usbhost

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        realsense2
        )
# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(
        usbhost
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE src)
