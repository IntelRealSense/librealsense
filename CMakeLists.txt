# ubuntu 12.04 LTS cmake version 2.8.7
# ubuntu 14.04 LTS cmake version 2.8.12.2
# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

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
    file(STRINGS "./include/librealsense/rs.h" REALSENSE_VERSION_${VER_COMPONENT} REGEX "#define RS_API_${VER_COMPONENT}_VERSION")
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

IF(DEFINED CMAKE_PREFIX_PATH)
  IF("${CMAKE_PREFIX_PATH}" MATCHES "/opt/ros")
    set(ROS_BUILD_TYPE TRUE)
  ENDIF()
ENDIF()

IF (${ROS_BUILD_TYPE})
  message(STATUS "Building in a ROS environment")
  project(librealsense)

  #################################
  # catkin specific configuration #
  #################################
  find_package(catkin REQUIRED)
  # The catkin_package macro generates cmake config files for your package
  # Declare things to be passed to dependent projects
  # INCLUDE_DIRS: uncomment this if you package contains header files
  # LIBRARIES: libraries you create in this project that dependent projects also need
  # CATKIN_DEPENDS: catkin_packages dependent projects also need
  # DEPENDS: system dependencies of this project that dependent projects also need
  catkin_package(
    INCLUDE_DIRS include
    LIBRARIES realsense
  )
ELSE()
  project(Realsense)
ENDIF()

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(REALSENSE_CPP
    src/archive.cpp
    src/context.cpp
    src/device.cpp
    src/ds-device.cpp
    src/ds-private.cpp
    src/f200.cpp
    src/hw-monitor.cpp
    src/image.cpp
    src/ivcam-private.cpp
    src/ivcam-device.cpp
    src/log.cpp
    src/motion-module.cpp
    src/r200.cpp
    src/rs.cpp
    src/sr300.cpp
    src/stream.cpp
    src/sync.cpp
    src/timestamps.cpp
    src/types.cpp
    src/uvc-libuvc.cpp
    src/uvc-v4l2.cpp
    src/uvc-wmf.cpp
    src/uvc.cpp
    src/verify.c
    src/zr300.cpp
)

set(REALSENSE_HPP
    src/archive.h
    src/context.h
    src/device.h
    src/ds-device.h
    src/ds-private.h
    src/f200.h
    src/hw-monitor.h
    src/image.h
    src/ivcam-private.h
    src/ivcam-device.h
    src/motion-module.h
    src/r200.h
    src/sr300.h
    src/stream.h
    src/sync.h
    src/timestamps.h
    src/types.h
    src/uvc.h
    src/zr300.h
)

if(WIN32)
    set(BACKEND RS_USE_WMF_BACKEND)
    set(REALSENSE_DEF CMake/realsense.def)
    # Makes VS15 find the DLL when trying to run examples/tests
    set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
elseif(APPLE)
    set(BACKEND RS_USE_LIBUVC_BACKEND)
else()
    set(BACKEND RS_USE_V4L2_BACKEND)
endif()
add_definitions(-D${BACKEND} -DUNICODE)

if(UNIX)
    list(APPEND REALSENSE_CPP
        src/libuvc/ctrl.c
        src/libuvc/dev.c
        src/libuvc/diag.c
        src/libuvc/frame.c
        src/libuvc/init.c
        src/libuvc/stream.c
    )
    list(APPEND REALSENSE_HPP
        src/libuvc/libuvc_config.h
        src/libuvc/libuvc.h
        src/libuvc/libuvc_internal.h
        src/libuvc/utlist.h
    )

    find_package(PkgConfig REQUIRED)
    find_package (Threads REQUIRED)
    pkg_search_module(LIBUSB1 REQUIRED libusb-1.0)
    if(LIBUSB1_FOUND)
      include_directories(SYSTEM ${LIBUSB1_INCLUDE_DIRS})
      link_directories(${LIBUSB1_LIBRARY_DIRS})
    else()
      message( FATAL_ERROR "Failed to find libusb-1.0" )
    endif(LIBUSB1_FOUND)

    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fPIC -pedantic -g -D_BSD_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -pedantic -g -Ofast -Wno-missing-field-initializers")
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

option(BUILD_SHARED_LIBS "Build shared library" ON)
if(BUILD_SHARED_LIBS)
    add_library(realsense SHARED ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_DEF})
    set_target_properties(realsense PROPERTIES VERSION ${REALSENSE_VERSION_STRING}
                                    SOVERSION ${REALSENSE_VERSION_MAJOR})
    target_link_libraries(realsense ${LIBUSB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
else()
    add_library(realsense STATIC ${REALSENSE_CPP} ${REALSENSE_HPP})
endif()

target_include_directories(realsense PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${LIBUSB1_INCLUDE_DIRS})

IF (${ROS_BUILD_TYPE})
    install(TARGETS realsense
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    )

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
ELSE()
    install( TARGETS realsense
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/librealsense DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
ENDIF()

option(BUILD_EXAMPLES "Build realsense examples." OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(BUILD_UNIT_TESTS "Build realsense unit tests." ON)
if(BUILD_UNIT_TESTS)
  add_subdirectory(unit-tests)
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
if(${ignore_count} GREATER 0)
  if(${ignore_count} GREATER 1)
    message ( WARNING "${ignore_count} source files ignored: ${AllSources}" )
  else(${ignore_count} GREATER 1)
    message ( WARNING "Source file ignored: ${AllSources}" )
  endif(${ignore_count} GREATER 1)
endif(${ignore_count} GREATER 0)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake" "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
