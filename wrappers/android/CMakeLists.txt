# used to build libirsajni.so
# Thanks CMakeLists.txt in the ART-OF-DESIGN official librealsense project

#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

macro(info msg)
    message(STATUS "Info: ${msg}")
endmacro()

macro(infoValue variableName)
    info("${variableName}=\${${variableName}}")
endmacro()

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_STANDARD 11)
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
    unset(WIN32)
    unset(UNIX)
    unset(APPLE)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fPIC -pedantic -g -D_BSD_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pedantic -g -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-multichar")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")
    message(STATUS "Prepare Intel RealSense SDK for Android OS (${ANDROID_NDK_ABI_NAME})")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(IRSA_CPP
    src/jni/jni_buildversion.cpp
    src/jni/jni_utils.cpp
    src/jni/android_irsa_IrsaMgr.cpp
    src/jni/android_irsa_IrsaPreview.cpp
    src/native/irsa_mgr.cpp
    src/native/irsa_preview.cpp
    src/native/irsa_preview_factory.cpp
    src/native/irsa_rs_preview.cpp
    src/native/util.cpp
    src/native/cde_log.c
    src/native/fifo.c
)


set(IRSA_HPP
    src/jni/android_irsa_IrsaMgr.h
    src/jni/android_irsa_IrsaPreview.h
    src/jni/jni_utils.h
    src/jni/jni_buildversion.h
    src/native/fifo.h
    src/native/cde_log.h
    src/native/irsa_mgr.h
    src/native/irsa_preview.h
    src/native/irsa_fake_preview.h
    src/native/irsa_rs_preview.h
    src/native/irsa_preview_factory.h
    src/native/irsa_event.h
    src/native/util.h
)

option(BUILD_WITH_STATIC_CRT "Build with static link CRT" ON)

#enable detailed log/trace output
add_definitions(-D__IRSA_DEBUG__)
#using C++ 11's smart pointer as default behaviour
add_definitions(-D__SMART_POINTER__)

option(BUILD_SHARED_LIBS "Build shared library" ON)
if(BUILD_SHARED_LIBS)
        add_library(irsajni SHARED ${IRSA_CPP} ${IRSA_HPP} ${IRSA_CU} ${IRSA_CUH} ${IRSA_DEF})
else()
        add_library(irsajni STATIC ${IRSA_CPP} ${IRSA_HPP} ${IRSA_CU} ${IRSA_CUH})
endif()


add_library(realsense2-lib STATIC IMPORTED )
set_target_properties( 
                       realsense2-lib
                       PROPERTIES IMPORTED_LOCATION

                       ${BASEPATH}/out/target/${ANDROID_ABI}/build_librealsense/librealsense2.a )

add_library(realsense2-usb-lib STATIC IMPORTED )
set_target_properties( 
                       realsense2-usb-lib
                       PROPERTIES IMPORTED_LOCATION

                       ${BASEPATH}/out/target/${ANDROID_ABI}/build_librealsense/libusb_install/lib/libusb.a)

add_library(realsense2-file-lib STATIC IMPORTED )
set_target_properties( 
                       realsense2-file-lib
                       PROPERTIES IMPORTED_LOCATION

                       ${BASEPATH}/out/target/${ANDROID_ABI}/build_librealsense/third-party/realsense-file/librealsense-file.a)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries( 
    irsajni

    realsense2-lib realsense2-usb-lib realsense2-file-lib
    ${log-lib}
    ${android-lib} 
)

add_definitions(-DELPP_THREAD_SAFE)
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)

include_directories(irsajni src src/native src/native/androidsrc src/jni ../../../librealsense/include src/native/include)

set_target_properties (irsajni PROPERTIES
    FOLDER Library
)

set(LRS_LIB_NAME irsajni)


# Check for unreferenced files
FILE(GLOB_RECURSE AllSources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "src/*.c" "src/*.cpp" "src/*.cc" "src/*.c++" "src/*.cu"
  "src/*.h" "src/*.hpp" "src/*.hh" "src/*.h++" "src/*.cuh")
#message ( "${AllSources}" )
#message ( "${IRSA_CPP}" )
#message ( "${IRSA_HPP}" )
list(LENGTH AllSources ignore_count)

