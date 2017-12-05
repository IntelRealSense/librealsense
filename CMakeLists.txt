#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(librealsense2)

macro(info msg)
    message(STATUS "Info: ${msg}")
endmacro()

macro(infoValue variableName)
    info("${variableName}=\${${variableName}}")
endmacro()

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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

option(BUILD_WITH_OPENMP "Use OpenMP" ON)
if(BUILD_WITH_OPENMP)
    find_package(OpenMP)
    if(NOT OPENMP_FOUND)
        message(FATAL_ERROR "\n\n OpenMP package is missing!\n\n")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
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
    src/ivcam/ivcam-private.cpp
    src/log.cpp
    src/rs.cpp
    src/ivcam/sr300.cpp
    src/types.cpp
    src/linux/backend-v4l2.cpp
    src/linux/backend-hid.cpp
    src/backend.cpp
    src/verify.c
    src/proc/align.cpp
    src/proc/colorizer.cpp
    src/proc/pointcloud.cpp
    src/proc/synthetic-stream.cpp
    src/proc/syncer-processing-block.cpp
    src/proc/decimation-filter.cpp
    src/proc/spatial-filter.cpp
    src/proc/temporal-filter.cpp
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


    src/ds5/advanced_mode/rs_advanced_mode.cpp
    src/ds5/advanced_mode/presets.cpp
    src/ds5/advanced_mode/advanced_mode.cpp

    third-party/easyloggingpp/src/easylogging++.cc
    third-party/sqlite/sqlite3.c

    src/mock/sql.cpp
    src/mock/recorder.cpp

    src/media/record/record_device.cpp
    src/media/record/record_sensor.cpp
    src/media/playback/playback_device.cpp
    src/media/playback/playback_sensor.cpp
    )

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
    src/proc/synthetic-stream.h
    src/proc/decimation-filter.h
    src/proc/spatial-filter.h
    src/proc/temporal-filter.h
    src/proc/syncer-processing-block.h
    src/algo.h
    src/option.h
    src/metadata.h
    src/metadata-parser.h
    src/error-handling.h
    src/hw-monitor.h
    src/image.h
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

    src/media/record/record_device.h
    src/media/record/record_sensor.h
    src/media/playback/playback_device.h
    src/media/playback/playback_sensor.h
    src/media/ros/ros_reader.h
    src/media/ros/ros_writer.h

    src/ds5/advanced_mode/json_loader.hpp
    src/ds5/advanced_mode/presets.h

    third-party/easyloggingpp/src/easylogging++.h
    third-party/sqlite/sqlite3.h
    src/mock/sql.h
    src/mock/recorder.h

    src/media/ros/ros_file_format.h
)

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
        include/librealsense2/h/rs_streaming.h
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
        src/proc/decimation-filter.cpp
        src/proc/spatial-filter.cpp
        src/proc/temporal-filter.cpp
        src/proc/syncer-processing-block.cpp
        )

    source_group("Header Files\\Processing Blocks" FILES
        src/proc/colorizer.h
        src/proc/align.h
        src/proc/pointcloud.h
        src/proc/synthetic-stream.h
        src/proc/decimation-filter.h
        src/proc/spatial-filter.h
        src/proc/temporal-filter.h
        src/proc/syncer-processing-block.h
        )

    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
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
    src/libuvc/ctrl.c
    src/libuvc/ctrl-gen.c
    src/libuvc/device.c
    src/libuvc/diag.c
    src/libuvc/frame.c
    src/libuvc/init.c
    src/libuvc/misc.c
    src/libuvc/stream.c
    src/libuvc/libuvc.cpp
    )
    
    list(APPEND REALSENSE_HPP
    src/libuvc/libuvc.h
    src/libuvc/libuvc_internal.h
    src/libuvc/utlist.h
    src/libuvc/libuvc_config.h
    )
    
    message( WARNING "Using libuvc!" )
endif()

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4819")
    add_definitions(-D_UNICODE)
endif()

add_definitions(-D${BACKEND} -DUNICODE)

if(ANDROID)
    set(ANDROID_STL "c++_static")

    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fPIC -pedantic -g -D_BSD_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pedantic -g -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-multichar")

    set(LIBUSB1_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libusb/libusb")
    set(LIBUSB1_LIBRARY_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/libusb/android/libs/${ANDROID_NDK_ABI_NAME}")
    set(LIBUSB1_LIBRARIES "usb1.0")

    include_directories(SYSTEM ${LIBUSB1_INCLUDE_DIRS})
    link_directories(${LIBUSB1_LIBRARY_DIRS})
elseif(UNIX)
    find_package(PkgConfig)
    if(NOT PKG_CONFIG_FOUND)
        message(FATAL_ERROR "\n\n PkgConfig package is missing!\n\n")
    endif()

    find_package (Threads REQUIRED)
    list(APPEND librealsense_PKG_DEPS "Threads")

    pkg_search_module(LIBUSB1 REQUIRED libusb-1.0)
    if(LIBUSB1_FOUND)
        include_directories(SYSTEM ${LIBUSB1_INCLUDE_DIRS})
        link_directories(${LIBUSB1_LIBRARY_DIRS})
        list(APPEND librealsense_PKG_DEPS "libusb-1.0")
    else()
        message( FATAL_ERROR "Failed to find libusb-1.0" )
    endif(LIBUSB1_FOUND)

    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fPIC -pedantic -g -D_BSD_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pedantic -g -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-multichar")

    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE MACHINE)
    if(${MACHINE} MATCHES "arm-linux-gnueabihf")
      set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mfpu=neon -mfloat-abi=hard -ftree-vectorize")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon -mfloat-abi=hard -ftree-vectorize")
    elseif(${MACHINE} MATCHES "aarch64-linux-gnu")
      set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mstrict-align -ftree-vectorize")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstrict-align -ftree-vectorize")
    else(${MACHINE} MATCHES "arm-linux-gnueabihf")
      set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mssse3")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
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

add_subdirectory(third-party/realsense-file)

option(BUILD_SHARED_LIBS "Build shared library" ON)
if(BUILD_SHARED_LIBS)
    if(WIN32)
        add_library(realsense2 SHARED
            ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_DEF}
            src/res/resource.h
            src/res/librealsense.rc)

        source_group("Resources" FILES
        src/res/resource.h
        src/res/librealsense.rc
        )
    else()
        add_library(realsense2 SHARED ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_DEF})
    endif()
else()
    add_library(realsense2 STATIC ${REALSENSE_CPP} ${REALSENSE_HPP})
endif()

set_target_properties(realsense2 PROPERTIES VERSION ${REALSENSE_VERSION_STRING}
                                SOVERSION ${REALSENSE_VERSION_MAJOR})
target_link_libraries(realsense2 PRIVATE realsense-file ${LIBUSB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
list(APPEND librealsense_PKG_LIBS ${CMAKE_THREAD_LIBS_INIT})

add_definitions(-DELPP_THREAD_SAFE)
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)
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

target_include_directories(realsense2 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                            $<INSTALL_INTERFACE:include>
                                     PRIVATE ${LIBUSB1_INCLUDE_DIRS})

set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/realsense2")

install(TARGETS realsense2
EXPORT realsense2Targets
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/librealsense2 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(CMake/realsense2Config.cmake.in realsense2Config.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/bin
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR)

install(EXPORT realsense2Targets FILE realsense2Targets.cmake NAMESPACE realsense2::
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/realsense2Config.cmake"
    DESTINATION ${CMAKECONFIG_INSTALL_DIR})


install(CODE "execute_process(COMMAND ldconfig)")

option(BUILD_UNIT_TESTS "Build realsense unit tests." ON)
option(BUILD_EXAMPLES "Build realsense examples and tools." ON)
option(ENFORCE_METADATA "Require WinSDK with Metadata support during compilation. Windows OS Only" OFF)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)
option(BUILD_CV_EXAMPLES "Build OpenCV examples" OFF)
option(BUILD_PCL_EXAMPLES "Build PCL examples" OFF)
option(BUILD_NODEJS_BINDINGS "Build Node.js bindings" OFF)

# This parameter is meant for disabling graphical examples when building for
# save-to-disk targets.
option(BUILD_GRAPHICAL_EXAMPLES "Build graphical examples and tools." ON)

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

if (BUILD_PYTHON_BINDINGS)
  add_subdirectory(wrappers/python)
endif()

if (BUILD_NODEJS_BINDINGS)
  add_subdirectory(wrappers/nodejs)
endif()

if (BUILD_CV_EXAMPLES)
  add_subdirectory(wrappers/opencv)
endif()

if (BUILD_PCL_EXAMPLES)
  add_subdirectory(wrappers/pcl)
endif()

# Check for unreferenced files
FILE(GLOB_RECURSE AllSources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "src/*.c" "src/*.cpp" "src/*.cc" "src/*.c++"
  "src/*.h" "src/*.hpp" "src/*.hh" "src/*.h++")
#message ( "${AllSources}" )
#message ( "${REALSENSE_CPP}" )
#message ( "${REALSENSE_HPP}" )
foreach(afile ${REALSENSE_CPP})
  list(REMOVE_ITEM AllSources ${afile})
endforeach(afile)
foreach(afile ${REALSENSE_HPP})
  list(REMOVE_ITEM AllSources ${afile})
endforeach(afile)
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
