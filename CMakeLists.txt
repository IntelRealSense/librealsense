#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

macro(info msg)
    message(STATUS "Info: ${msg}")
endmacro()

macro(infoValue variableName)
    info("${variableName}=\${${variableName}}")
endmacro()

option(BUILD_WITH_CUDA "Enable CUDA" OFF)

if(BUILD_WITH_CUDA)
	info("Building with CUDA requires CMake v3.8+")
	cmake_minimum_required(VERSION 3.8.0)
    project(librealsense2 LANGUAGES CXX C CUDA)
else()
    project(librealsense2 LANGUAGES CXX C)
endif()


##################################################################
# Parse librealsense version and assign it to CMake variables    #
# This function parses librealsense public API header file, rs.h #
# and retrieves version numbers embedded in the source code.     #
# Since the function relies on hard-coded variables, it is prone #
# for failures should these constants be modified in future      #
##################################################################
function(assign_version_property VER_COMPONENT)
    file(STRINGS "./include/librealsense2/rs.h" REALSENSE_VERSION_${VER_COMPONENT} REGEX "#define RS2_API_${VER_COMPONENT}_VERSION")
    separate_arguments(REALSENSE_VERSION_${VER_COMPONENT})
    list(GET REALSENSE_VERSION_${VER_COMPONENT} -1 tmp)
    if (tmp LESS 0)
        message( FATAL_ERROR "Could not obtain valid Librealsense version ${VER_COMPONENT} component - actual value is ${tmp}" )
    endif()
    set(REALSENSE_VERSION_${VER_COMPONENT} ${tmp} PARENT_SCOPE)
endfunction()

set(REALSENSE_VERSION_MAJOR -1)
set(REALSENSE_VERSION_MINOR -1)
set(REALSENSE_VERSION_PATCH -1)
assign_version_property(MAJOR)
assign_version_property(MINOR)
assign_version_property(PATCH)
set(REALSENSE_VERSION_STRING ${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}.${REALSENSE_VERSION_PATCH})
infoValue(REALSENSE_VERSION_STRING)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)   
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CUDA_STANDARD 11)
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

# This parameter is meant for disabling graphical examples when building for
# save-to-disk targets.
option(BUILD_GRAPHICAL_EXAMPLES "Build graphical examples and tools." ON)

option(BUILD_WITH_OPENMP "Use OpenMP" OFF)

option(USE_SYSTEM_LIBUSB "Use the distribution libusb package" ON)
if (WIN32)
    set(USE_SYSTEM_LIBUSB OFF)
endif()

option(ENABLE_ZERO_COPY "Enable zero copy functionality" OFF)
if (ENABLE_ZERO_COPY)
    add_definitions(-DZERO_COPY)
endif()

if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
    unset(WIN32)
    unset(UNIX)
    unset(APPLE)
    set(USE_SYSTEM_LIBUSB OFF)
    set(BUILD_WITH_OPENMP OFF)
    set(FORCE_LIBUVC ON)
    set(BUILD_GRAPHICAL_EXAMPLES OFF)
    set(ANDROID_STL "c++_static")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fPIC -pedantic -g -D_BSD_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pedantic -g -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-multichar")

    message(STATUS "Prepare RealSense SDK for Android OS (${ANDROID_NDK_ABI_NAME})")
endif()

if(BUILD_WITH_OPENMP)
    find_package(OpenMP)
    if(NOT OPENMP_FOUND)
        message(FATAL_ERROR "\n\n OpenMP package is missing!\n\n")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    endif()
elseif(UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()


option(BUILD_WITH_TM2 "Build with support for Intel TM2 tracking device" OFF)

option(BUILD_EASYLOGGINGPP "Build EasyLogging++ as a part of the build" ON)

if (BUILD_EASYLOGGINGPP)
    add_definitions(-DBUILD_EASYLOGGINGPP)
endif()

if (BUILD_WITH_CUDA)
    find_package(CUDA REQUIRED)
    if(CUDA_FOUND)
        include_directories(${CUDA_INCLUDE_DIRS})
        SET(ALL_CUDA_LIBS ${CUDA_LIBRARIES} ${CUDA_cusparse_LIBRARY} ${CUDA_cublas_LIBRARY})
        SET(LIBS ${LIBS} ${ALL_CUDA_LIBS})
        message(STATUS "CUDA_LIBRARIES: ${CUDA_INCLUDE_DIRS} ${ALL_CUDA_LIBS}")
        set(CUDA_PROPAGATE_HOST_FLAGS OFF)
        set(CUDA_SEPARABLE_COMPILATION ON)
        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS}; -O3 -gencode arch=compute_53,code=sm_53
        -gencode arch=compute_62,code=sm_62;")
    endif()
endif()


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(REALSENSE_CPP
    src/environment.cpp
    src/device_hub.cpp
    src/pipeline.cpp
    src/archive.cpp
    src/context.cpp
    src/device.cpp
    src/sensor.cpp
    src/algo.cpp
    src/sync.cpp
    src/stream.cpp
    src/option.cpp
    src/error-handling.cpp
    src/hw-monitor.cpp
    src/image.cpp
    src/image_avx.cpp
    src/ivcam/ivcam-private.cpp
    src/log.cpp
    src/rs.cpp
    src/ivcam/sr300.cpp
    src/types.cpp
    src/linux/backend-v4l2.cpp
    src/linux/backend-hid.cpp
    src/backend.cpp
    src/verify.c
    src/software-device.cpp
    src/proc/align.cpp
    src/proc/colorizer.cpp
    src/proc/pointcloud.cpp
    src/proc/occlusion-filter.cpp
    src/proc/synthetic-stream.cpp
    src/proc/syncer-processing-block.cpp
    src/proc/decimation-filter.cpp
    src/proc/spatial-filter.cpp
    src/proc/temporal-filter.cpp
    src/proc/hole-filling-filter.cpp
    src/proc/disparity-transform.cpp
    src/source.cpp
    src/ds5/ds5-options.cpp
    src/ds5/ds5-timestamp.cpp
    src/ds5/ds5-private.cpp
    src/ds5/ds5-motion.cpp
    src/ds5/ds5-rolling-shutter.cpp
    src/ds5/ds5-device.cpp
    src/ds5/ds5-color.cpp
    src/ds5/ds5-active.cpp
    src/ds5/ds5-factory.cpp
    src/win/win-helpers.cpp
    src/win/win-uvc.cpp
    src/win/win-usb.cpp
    src/win/win-hid.cpp
    src/win/win-backend.cpp

    src/l500/l500.cpp
    src/l500/l500-private.cpp

    src/ds5/advanced_mode/rs_advanced_mode.cpp
    src/ds5/advanced_mode/presets.cpp
    src/ds5/advanced_mode/advanced_mode.cpp

    third-party/sqlite/sqlite3.c

    src/mock/sql.cpp
    src/mock/recorder.cpp

    src/media/record/record_device.cpp
    src/media/record/record_sensor.cpp
    src/media/playback/playback_device.cpp
    src/media/playback/playback_sensor.cpp
    )
    
if (BUILD_WITH_CUDA)
    set(REALSENSE_CU
        src/cuda/cuda-conversion.cu
        src/cuda/cuda-pointcloud.cu
        )

    set(REALSENSE_CUH
        src/cuda/cuda-conversion.cuh
        src/cuda/cuda-pointcloud.cuh
        )
endif()

set(REALSENSE_HPP
    include/librealsense2/rs.hpp
    include/librealsense2/rs.h

    include/librealsense2/h/rs_types.h
    include/librealsense2/h/rs_context.h
    include/librealsense2/h/rs_device.h
    include/librealsense2/h/rs_frame.h
    include/librealsense2/h/rs_option.h
    include/librealsense2/h/rs_processing.h
    include/librealsense2/h/rs_record_playback.h
    include/librealsense2/h/rs_sensor.h
    include/librealsense2/h/rs_internal.h
    include/librealsense2/h/rs_pipeline.h

    include/librealsense2/hpp/rs_types.hpp
    include/librealsense2/hpp/rs_context.hpp
    include/librealsense2/hpp/rs_device.hpp
    include/librealsense2/hpp/rs_frame.hpp
    include/librealsense2/hpp/rs_processing.hpp
    include/librealsense2/hpp/rs_record_playback.hpp
    include/librealsense2/hpp/rs_sensor.hpp
    include/librealsense2/hpp/rs_internal.hpp
    include/librealsense2/hpp/rs_pipeline.hpp

    include/librealsense2/rsutil.h
    include/librealsense2/rs_advanced_mode.h
    include/librealsense2/rs_advanced_mode.hpp
    include/librealsense2/h/rs_advanced_mode_command.h

    src/res/resource.h

    src/core/streaming.h
    src/core/debug.h
    src/core/advanced_mode.h
    src/core/roi.h
    src/core/motion.h
    src/core/video.h
    src/core/options.h
    src/core/info.h
    src/core/extension.h
    src/core/processing.h
    src/software-device.h

    src/environment.h
    src/device_hub.h
    src/pipeline.h
    src/config.h
    src/archive.h
    src/concurrency.h
    src/context.h
    src/sensor.h
    src/sync.h
    src/sensor.h
    src/stream.h
    src/proc/align.h
    src/proc/colorizer.h
    src/proc/pointcloud.h
    src/proc/occlusion-filter.h
    src/proc/synthetic-stream.h
    src/proc/decimation-filter.h
    src/proc/spatial-filter.h
    src/proc/temporal-filter.h
    src/proc/hole-filling-filter.h
    src/proc/syncer-processing-block.h
    src/proc/disparity-transform.h
    src/algo.h
    src/option.h
    src/metadata.h
    src/metadata-parser.h
    src/error-handling.h
    src/hw-monitor.h
    src/image.h
    src/image_avx.h
    src/source.h
    src/ivcam/ivcam-private.h
    src/types.h
    src/backend.h
    src/device.h
    src/ivcam/sr300.h
    src/ds5/ds5-factory.h
    src/ds5/ds5-device.h
    src/ds5/ds5-options.h
    src/ds5/ds5-private.h
    src/ds5/ds5-timestamp.h
    src/ds5/ds5-active.h
    src/ds5/ds5-motion.h
    src/ds5/ds5-rolling-shutter.h
    src/ds5/ds5-color.h
    src/linux/backend-v4l2.h
    src/linux/backend-hid.h
    src/win/win-helpers.h
    src/win/win-uvc.h
    src/win/win-usb.h
    src/win/win-hid.h
    src/win/win-backend.h
    src/api.h
    src/core/serialization.h

    src/l500/l500.h
    src/l500/l500-private.h

    src/media/record/record_device.h
    src/media/record/record_sensor.h
    src/media/playback/playback_device.h
    src/media/playback/playback_sensor.h
    src/media/ros/ros_reader.h
    src/media/ros/ros_writer.h

    src/ds5/advanced_mode/json_loader.hpp
    src/ds5/advanced_mode/presets.h

    third-party/sqlite/sqlite3.h
    src/mock/sql.h
    src/mock/recorder.h

    src/media/ros/ros_file_format.h
)

if(BUILD_EASYLOGGINGPP)
    list(APPEND REALSENSE_CPP third-party/easyloggingpp/src/easylogging++.cc)
    list(APPEND REALSENSE_HPP third-party/easyloggingpp/src/easylogging++.h)
endif()

option(BUILD_WITH_STATIC_CRT "Build with static link CRT" ON)

if(WIN32)
    source_group("Source Files\\Backend" FILES
        src/win/win-helpers.cpp
        src/win/win-uvc.cpp
        src/win/win-usb.cpp
        src/win/win-hid.cpp
        src/win/win-backend.cpp
        src/linux/backend-v4l2.cpp
        src/linux/backend-hid.cpp
        src/backend.cpp
        )

    source_group("Source Files\\Logging" FILES
        third-party/easyloggingpp/src/easylogging++.cc
        src/log.cpp
        )

    source_group("Source Files\\Media" FILES
        src/media/record/record_device.cpp
        src/media/record/record_sensor.cpp
        src/media/playback/playback_device.cpp
        src/media/playback/playback_sensor.cpp
        )

    source_group("Header Files\\Backend" FILES
        src/win/win-helpers.h
        src/win/win-uvc.h
        src/win/win-usb.h
        src/win/win-hid.h
        src/win/win-backend.h
        src/linux/backend-v4l2.h
        src/linux/backend-hid.h
        src/backend.h)

    source_group("Header Files\\API" FILES
        include/librealsense2/rs.hpp
        include/librealsense2/rs.h
        CMake/realsense.def
        )

    source_group("Header Files\\API\\C Headers" FILES
        include/librealsense2/h/rs_context.h
        include/librealsense2/h/rs_device.h
        include/librealsense2/h/rs_frame.h
        include/librealsense2/h/rs_types.h
        include/librealsense2/h/rs_sensor.h
        include/librealsense2/h/rs_option.h
        include/librealsense2/h/rs_processing.h
        include/librealsense2/h/rs_record_playback.h
        include/librealsense2/h/rs_pipeline.h
        include/librealsense2/h/rs_internal.h

        include/librealsense2/rsutil.h
        include/librealsense2/rs_advanced_mode.h
        include/librealsense2/h/rs_advanced_mode_command.h
        )

    source_group("Header Files\\API\\C++ Headers" FILES
        include/librealsense2/hpp/rs_types.hpp
        include/librealsense2/hpp/rs_context.hpp
        include/librealsense2/hpp/rs_device.hpp
        include/librealsense2/hpp/rs_frame.hpp
        include/librealsense2/hpp/rs_processing.hpp
        include/librealsense2/hpp/rs_pipeline.hpp
        include/librealsense2/hpp/rs_record_playback.hpp
        include/librealsense2/hpp/rs_sensor.hpp
        include/librealsense2/hpp/rs_internal.hpp

        include/librealsense2/rs_advanced_mode.hpp
        )

    source_group("Header Files\\Recorder" FILES
        third-party/sqlite/sqlite3.h
        src/mock/sql.h
        src/mock/recorder.h
        )

    source_group("Source Files\\Recorder" FILES
        third-party/sqlite/sqlite3.c
        src/mock/sql.cpp
        src/mock/recorder.cpp
        )

    source_group("Source Files\\API" FILES
        src/rs.cpp
        src/verify.c
        src/ds5/advanced_mode/rs_advanced_mode.cpp
        )

    source_group("Header Files\\Media" FILES
        src/media/record/record_device.h
        src/media/record/record_sensor.h
        src/media/playback/playback_device.h
        src/media/playback/playback_sensor.h
        )
    source_group("Header Files\\Media\\Ros Serializer" FILES
        src/media/ros/ros_reader.h
        src/media/ros/ros_writer.h
        src/media/ros/ros_file_format.h
        )

    source_group("Source Files\\Devices" FILES
        src/l500/l500.cpp
        src/l500/l500-private.cpp

        src/ds5/ds5-device.cpp
        src/ds5/ds5-factory.cpp
        src/ds5/ds5-options.cpp
        src/ds5/ds5-private.cpp
        src/ds5/ds5-timestamp.cpp
        src/ds5/ds5-motion.cpp
        src/ds5/ds5-rolling-shutter.cpp
        src/ds5/ds5-color.cpp
        src/ds5/ds5-active.cpp
        src/ivcam/sr300.cpp
        src/ivcam/ivcam-private.cpp
        src/software-device.cpp
        )

    source_group("Source Files\\Devices\\Advanced Mode" FILES
        src/ds5/advanced_mode/rs_advanced_mode.cpp
        src/ds5/advanced_mode/presets.cpp
        src/ds5/advanced_mode/advanced_mode.cpp
        )

    source_group("Header Files\\Devices\\Advanced Mode" FILES
        src/ds5/advanced_mode/json_loader.hpp
        src/ds5/advanced_mode/presets.h
        )

    source_group("Header Files\\Devices" FILES
        src/l500/l500.h
        src/l500/l500-private.h

        src/ds5/ds5-device.h
        src/ds5/ds5-options.h
        src/ds5/ds5-private.h
        src/ds5/ds5-timestamp.h
        src/ds5/ds5-factory.h
        src/ds5/ds5-active.h
        src/ds5/ds5-motion.h
        src/ds5/ds5-rolling-shutter.h
        src/ds5/ds5-color.h
        src/ivcam/sr300.h
        src/ivcam/ivcam-private.h
        src/software-device.h
        )

    source_group("Header Files\\Core" FILES
        src/core/streaming.h
        src/core/debug.h
        src/core/roi.h
        src/core/video.h
        src/core/motion.h
        src/core/info.h
        src/core/options.h
        src/core/extension.h
        src/core/processing.h
        )

    source_group("Header Files\\Logging" FILES
        third-party/easyloggingpp/src/easylogging++.h
        )

    source_group("Source Files\\Processing Blocks" FILES
        src/proc/colorizer.cpp
        src/proc/synthetic-stream.cpp
        src/proc/align.cpp
        src/proc/pointcloud.cpp
        src/proc/occlusion-filter.cpp
        src/proc/decimation-filter.cpp
        src/proc/spatial-filter.cpp
        src/proc/temporal-filter.cpp
        src/proc/hole-filling-filter.cpp
        src/proc/syncer-processing-block.cpp
        src/proc/disparity-transform.cpp
        )

    source_group("Header Files\\Processing Blocks" FILES
        src/proc/colorizer.h
        src/proc/align.h
        src/proc/pointcloud.h
        src/proc/occlusion-filter.h
        src/proc/synthetic-stream.h
        src/proc/decimation-filter.h
        src/proc/spatial-filter.h
        src/proc/temporal-filter.h
        src/proc/syncer-processing-block.h
        src/proc/disparity-transform.h
        src/proc/hole-filling-filter.h
        )

    if(BUILD_WITH_STATIC_CRT)
        foreach(flag_var
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MD")
        endforeach(flag_var)
    endif()
else()

endif()

option(FORCE_LIBUVC "Explicitly turn-on libuvc backend" OFF)

if(WIN32)
    set(BACKEND RS2_USE_WMF_BACKEND)
    set(REALSENSE_DEF CMake/realsense.def)
    # Makes VS15 find the DLL when trying to run examples/tests
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    # build with multiple cores
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    add_definitions(-D__SSSE3__ -D_CRT_SECURE_NO_WARNINGS)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
elseif(APPLE)
    set(FORCE_LIBUVC ON)
else()
    set(BACKEND RS2_USE_V4L2_BACKEND)
endif()

if(FORCE_LIBUVC)
    set(BACKEND RS2_USE_LIBUVC_BACKEND)

    list(APPEND REALSENSE_CPP
    src/libuvc/ctrl.cpp
    src/libuvc/ctrl-gen.cpp
    src/libuvc/device.cpp
    src/libuvc/diag.cpp
    src/libuvc/frame.cpp
    src/libuvc/init.cpp
    src/libuvc/misc.cpp
    src/libuvc/stream.cpp
    src/libuvc/libuvc.cpp
    )

    list(APPEND REALSENSE_HPP
    src/libuvc/libuvc.h
    src/libuvc/libuvc_internal.h
    src/libuvc/utlist.h
    src/libuvc/libuvc_config.h
    )

    source_group("Header Files\\LibUvc" FILES
    src/libuvc/libuvc.h
    src/libuvc/libuvc_internal.h
    src/libuvc/utlist.h
    src/libuvc/libuvc_config.h
    )

    source_group("Source Files\\LibUvc" FILES
        src/libuvc/ctrl.cpp
        src/libuvc/ctrl-gen.cpp
        src/libuvc/device.cpp
        src/libuvc/diag.cpp
        src/libuvc/frame.cpp
        src/libuvc/init.cpp
        src/libuvc/misc.cpp
        src/libuvc/stream.cpp
        src/libuvc/libuvc.cpp
    )

    message( WARNING "Using libuvc!" )
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4819")
    add_definitions(-D_UNICODE)
    set(LRS_TRY_USE_AVX true)
endif()

add_definitions(-D${BACKEND} -DUNICODE)

if(UNIX)
    find_package(PkgConfig)
    if(NOT PKG_CONFIG_FOUND)
        message(FATAL_ERROR "\n\n PkgConfig package is missing!\n\n")
    endif()

    if (USE_SYSTEM_LIBUSB)
        pkg_search_module(LIBUSB1 REQUIRED libusb-1.0)
        if(LIBUSB1_FOUND)
            include_directories(SYSTEM ${LIBUSB1_INCLUDE_DIRS})
            link_directories(${LIBUSB1_LIBRARY_DIRS})
            list(APPEND librealsense_PKG_DEPS "libusb-1.0")
        else()
            message( FATAL_ERROR "Failed to find libusb-1.0" )
        endif(LIBUSB1_FOUND)
    endif()

    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fPIC -pedantic -g -D_BSD_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pedantic -g -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-multichar -Wsequence-point -Wformat-security")

    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE MACHINE)
    if(${MACHINE} MATCHES "arm-linux-gnueabihf")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mfpu=neon -mfloat-abi=hard -ftree-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon -mfloat-abi=hard -ftree-vectorize")
    elseif(${MACHINE} MATCHES "aarch64-linux-gnu")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mstrict-align -ftree-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstrict-align -ftree-vectorize")
    else()
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mssse3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
        set(LRS_TRY_USE_AVX true)
    endif(${MACHINE} MATCHES "arm-linux-gnueabihf")
endif()

# Set CMAKE_INSTALL_* if not defined
include(GNUInstallDirs)

option(TRACE_API "Log all C API calls" OFF)
if(TRACE_API)
    add_definitions(-DTRACE_API)
endif()

option(HWM_OVER_XU "Send HWM commands over UVC XU control" ON)
if(HWM_OVER_XU)
    add_definitions(-DHWM_OVER_XU)
endif()

if (NOT USE_SYSTEM_LIBUSB)
    if(NOT WIN32)
        add_subdirectory(third-party/libusb/)
    elseif(FORCE_LIBUVC)
        add_subdirectory(third-party/libusb/)
    endif()
endif()

add_subdirectory(third-party/realsense-file)


if (BUILD_WITH_TM2)
    message(STATUS "Building with TM2")

    if(WIN32)
        source_group("Source Files\\Devices\\Tracking" FILES
            src/tm2/tm-context.cpp
            src/tm2/tm-info.cpp
            src/tm2/tm-device.cpp
            )

        source_group("Header Files\\Devices\\Tracking" FILES
            src/tm2/tm-context.h
            src/tm2/tm-info.h
            src/tm2/tm-device.h
            src/tm2/tm-conversions.h
            src/tm2/controller_event_serializer.h
            )
    endif()

    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)
    find_package(libtm REQUIRED)

    if(BUILD_WITH_STATIC_CRT)
        foreach(flag_var
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MD")
        endforeach(flag_var)
    endif()

    list(APPEND REALSENSE_HPP
    src/tm2/tm-context.h
    src/tm2/tm-device.h
    src/tm2/tm-info.h
    src/tm2/tm-conversions.h
    src/tm2/controller_event_serializer.h
    )

    list(APPEND REALSENSE_CPP
    src/tm2/tm-context.cpp
    src/tm2/tm-device.cpp
    src/tm2/tm-info.cpp
    )

    include_directories(${LIBTM_INCLUDE_DIR})
    set(TRACKING_DEVICE_LIBS libtm)

    install(TARGETS libtm
        EXPORT realsense2Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

if(LRS_TRY_USE_AVX)
    set_source_files_properties(src/image_avx.cpp PROPERTIES COMPILE_FLAGS -mavx2)
endif()

option(BUILD_SHARED_LIBS "Build shared library" ON)
if(BUILD_SHARED_LIBS)
    if(WIN32)
        add_library(realsense2 SHARED
            ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_DEF} ${REALSENSE_CU} ${REALSENSE_CUH}
            src/res/resource.h
            src/res/librealsense.rc) 
        source_group("Resources" FILES
        src/res/resource.h
        src/res/librealsense.rc
        )
    else()
        add_library(realsense2 SHARED ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_CU} ${REALSENSE_CUH} ${REALSENSE_DEF})
    endif()
else()
        add_library(realsense2 STATIC ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_CU} ${REALSENSE_CUH})
endif()


if(BUILD_WITH_TM2)
    target_compile_definitions(realsense2 PRIVATE WITH_TRACKING=1 BUILD_STATIC=1)
endif()
set_target_properties(realsense2 PROPERTIES VERSION ${REALSENSE_VERSION_STRING}
                                SOVERSION ${REALSENSE_VERSION_MAJOR})
target_link_libraries(realsense2 PRIVATE realsense-file ${CMAKE_THREAD_LIBS_INIT} ${TRACKING_DEVICE_LIBS})

if (NOT USE_SYSTEM_LIBUSB)
    if(NOT WIN32)
        target_link_libraries(realsense2 PRIVATE usb)
    elseif(FORCE_LIBUVC)
        target_link_libraries(realsense2 PRIVATE usb)
    endif()
else()
    target_link_libraries(realsense2 PRIVATE ${LIBUSB1_LIBRARIES})
endif()


add_definitions(-DELPP_THREAD_SAFE)
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)


list(APPEND librealsense_PKG_LIBS ${CMAKE_THREAD_LIBS_INIT})


include_directories(realsense2 src)

set_target_properties (realsense2 PROPERTIES
    FOLDER Library
)

set(LRS_LIB_NAME realsense2)

# Add additional include directories to allow file to include rosbag headers
include(${CMAKE_CURRENT_LIST_DIR}/third-party/realsense-file/config.cmake)
target_include_directories(realsense2 PRIVATE
        ${ROSBAG_HEADER_DIRS}
        ${BOOST_INCLUDE_PATH}
        ${LZ4_INCLUDE_PATH}
        )

if (USE_SYSTEM_LIBUSB)
    target_include_directories(realsense2 PRIVATE ${LIBUSB1_INCLUDE_DIRS})
endif()

target_include_directories(realsense2 PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${USB_INCLUDE_DIRS})

set_target_properties(realsense2
  PROPERTIES
  PUBLIC_HEADER
  "include/librealsense2/rs.hpp;include/librealsense2/rs.h;include/librealsense2/h/rs_context.h;include/librealsense2/h/rs_device.h;include/librealsense2/h/rs_frame.h;include/librealsense2/h/rs_types.h;include/librealsense2/h/rs_sensor.h;include/librealsense2/h/rs_option.h;include/librealsense2/h/rs_processing.h;include/librealsense2/h/rs_record_playback.h;include/librealsense2/h/rs_pipeline.h;include/librealsense2/h/rs_internal.h;include/librealsense2/rsutil.h;include/librealsense2/rs_advanced_mode.h;include/librealsense2/h/rs_advanced_mode_command.h;include/librealsense2/hpp/rs_types.hpp;include/librealsense2/hpp/rs_context.hpp;include/librealsense2/hpp/rs_device.hpp;include/librealsense2/hpp/rs_frame.hpp;include/librealsense2/hpp/rs_processing.hpp;include/librealsense2/hpp/rs_pipeline.hpp;include/librealsense2/hpp/rs_record_playback.hpp;include/librealsense2/hpp/rs_sensor.hpp;include/librealsense2/hpp/rs_internal.hpp;include/librealsense2/rs_advanced_mode.hpp"
  )

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/realsense2")

install(TARGETS realsense2
EXPORT realsense2Targets
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/librealsense2"
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/librealsense2 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(CMake/realsense2Config.cmake.in realsense2Config.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/bin
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/realsense2ConfigVersion.cmake"
    VERSION ${REALSENSE_VERSION_STRING} COMPATIBILITY AnyNewerVersion)

install(EXPORT realsense2Targets FILE realsense2Targets.cmake NAMESPACE realsense2::
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/realsense2Config.cmake"
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/realsense2ConfigVersion.cmake"
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})

install(CODE "execute_process(COMMAND ldconfig)")

option(BUILD_UNIT_TESTS "Build realsense unit tests. Note that when enabled, additional tests data set will be downloaded from a web server and stored in a temp directory" OFF)
option(BUILD_EXAMPLES "Build realsense examples and tools." ON)
option(ENFORCE_METADATA "Require WinSDK with Metadata support during compilation. Windows OS Only" OFF)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)
option(BUILD_CSHARP_BINDINGS "Build C# bindings" OFF)
option(BUILD_UNITY_BINDINGS "Copy the unity project to the build folder with the required dependencies" OFF)
option(BUILD_CV_EXAMPLES "Build OpenCV examples" OFF)
option(BUILD_PCL_EXAMPLES "Build PCL examples" OFF)
option(BUILD_NODEJS_BINDINGS "Build Node.js bindings" OFF)
SET(DOTNET_VERSION_LIBRARY "3.5" CACHE STRING ".Net Version, defaulting to '3.5', the Unity wrapper currently supports only .NET 3.5")
SET(DOTNET_VERSION_EXAMPLES "4.0" CACHE STRING ".Net Version, defaulting to '4.0'")

add_subdirectory(wrappers)

if((BUILD_EXAMPLES AND BUILD_GRAPHICAL_EXAMPLES) AND WIN32)
  add_subdirectory(third-party/glfw)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
  add_subdirectory(tools)
endif()

if(BUILD_UNIT_TESTS)
  add_subdirectory(unit-tests)
endif()

if (ENFORCE_METADATA)
  add_definitions(-DENFORCE_METADATA)
endif()

if (BUILD_WITH_CUDA)
    add_definitions(-DRS2_USE_CUDA)
endif()


# Check for unreferenced files
FILE(GLOB_RECURSE AllSources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "src/*.c" "src/*.cpp" "src/*.cc" "src/*.c++" "src/*.cu"
  "src/*.h" "src/*.hpp" "src/*.hh" "src/*.h++" "src/*.cuh")
#message ( "${AllSources}" )
#message ( "${REALSENSE_CPP}" )
#message ( "${REALSENSE_HPP}" )
if (BUILD_WITH_CUDA)
    foreach(afile ${REALSENSE_CPP})
      list(REMOVE_ITEM AllSources ${afile})
    endforeach(afile)
    foreach(afile ${REALSENSE_HPP})
      list(REMOVE_ITEM AllSources ${afile})
    endforeach(afile)
    foreach(afile ${REALSENSE_CU})
      list(REMOVE_ITEM AllSources ${afile})
    endforeach(afile)
    foreach(afile ${REALSENSE_CUH})
      list(REMOVE_ITEM AllSources ${afile})
    endforeach(afile)
endif()
list(LENGTH AllSources ignore_count)
#if(${ignore_count} GREATER 0)
#  if(${ignore_count} GREATER 1)
#    message ( WARNING "${ignore_count} source files ignored: ${AllSources}" )
#  else(${ignore_count} GREATER 1)
#    message ( WARNING "Source file ignored: ${AllSources}" )
#  endif(${ignore_count} GREATER 1)
#endif(${ignore_count} GREATER 0)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Support pkg-config meta-data file
# Export library compile and link-time dependencies
foreach(arg ${librealsense_PKG_DEPS})
    set(LRS_PKG_DEPS "${LRS_PKG_DEPS} ${arg}")
endforeach()
foreach(arg ${librealsense_PKG_LIBS})
    set(LRS_PKG_LIBS "${LRS_PKG_LIBS} ${arg}")
endforeach()
# Set library pkgconfig file for facilitating 3rd party integration
configure_file(config/librealsense.pc.in config/realsense2.pc @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/config/realsense2.pc"     DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
# Update system cache with newly installed binaries/libraries
if(UNIX)
    install(CODE "execute_process(COMMAND ldconfig)")
endif()
