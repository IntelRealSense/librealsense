# ubuntu 12.04 LTS cmake version 2.8.7
# ubuntu 14.04 LTS cmake version 2.8.12.2
# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

IF(DEFINED ENV{CMAKE_PREFIX_PATH})
  IF($ENV{CMAKE_PREFIX_PATH} MATCHES "/opt/ros")
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
CHECK_CXX_COMPILER_FLAG("-std=c++11 -pthread" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -pthread")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

set(REALSENSE_CPP
    src/archive.cpp
    src/context.cpp
    src/device.cpp
    src/subdevice.cpp
    src/option.cpp
    src/hw-monitor.cpp
    src/image.cpp
    src/ivcam-private.cpp
    src/ivcam-device.cpp
    src/log.cpp
    src/rs.cpp
    src/sr300.cpp
    src/types.cpp
    src/uvc-v4l2.cpp
    src/backend.cpp
    src/verify.c
    src/ds5.cpp
    src/ds5c.cpp
    src/ds5t.cpp
    src/ds5d.cpp
    src/ds5-private.cpp
    src/win/win-helpers.cpp
    src/win/win-uvc.cpp
    src/win/win-usb.cpp
    src/win/win-backend.cpp

    third_party/sqlite/sqlite3.h
    src/sql.h
    src/recorder.h
)

set(REALSENSE_HPP
    include/librealsense/rs.hpp
    include/librealsense/rs.h
    include/librealsense/rscore.hpp
    include/librealsense/rsutil.h
    include/librealsense/rsutil.hpp

    src/archive.h
    src/concurrency.h
    src/context.h
    src/device.h
    src/subdevice.h
    src/option.h
    src/hw-monitor.h
    src/image.h
    src/ivcam-private.h
    src/ivcam-device.h
    src/types.h
    src/backend.h
    src/sr300.h
    src/ds5.h
    src/ds5c.h
    src/ds5t.h
    src/ds5d.h
    src/ds5-private.h
    src/win/win-helpers.h
    src/win/win-uvc.h
    src/win/win-usb.h
    src/win/win-backend.h

    third_party/sqlite/sqlite3.c
    src/sql.cpp
    src/recorder.cpp
)

if(WIN32)
    source_group("Source Files\\Backend" FILES
        src/win/win-helpers.cpp
        src/win/win-uvc.cpp
        src/win/win-usb.cpp
        src/win/win-backend.cpp
        src/uvc-v4l2.cpp
        src/backend.cpp
        src/verify.c)

    source_group("Header Files\\Backend" FILES
        src/win/win-helpers.h
        src/win/win-uvc.h
        src/win/win-usb.h
        src/win/win-backend.h
        src/backend.h)

    source_group("Header Files\\API" FILES
        include/librealsense/rs.hpp
        include/librealsense/rs.h
        include/librealsense/rscore.hpp
        include/librealsense/rsutil.h
        include/librealsense/rsutil.hpp
        CMake/realsense.def
        )

    source_group("Header Files\\Recorder" FILES
        third_party/sqlite/sqlite3.h
        src/sql.h
        src/recorder.h
        )

    source_group("Source Files\\API" FILES
        src/rs.cpp
        )

    source_group("Source Files\\Recorder" FILES
        third_party/sqlite/sqlite3.c
        src/sql.cpp
        src/recorder.cpp
        )

    source_group("Source Files\\Devices" FILES
        src/ds5.cpp
        src/ds5c.cpp
        src/ds5t.cpp
        src/ds5d.cpp
        src/ds5-private.cpp
        src/sr300.cpp
        src/ivcam-private.cpp
        src/ivcam-device.cpp
        )

    source_group("Header Files\\Devices" FILES
        src/ds5.h
        src/ds5c.h
        src/ds5t.h
        src/ds5d.h
        src/ds5-private.h
        src/sr300.h
        src/ivcam-private.h
        src/ivcam-device.h
        )

else()

endif()

if(WIN32)
    set(BACKEND RS_USE_WMF_BACKEND)
    set(REALSENSE_DEF CMake/realsense.def)
    # Makes VS15 find the DLL when trying to run examples/tests
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    add_definitions(-D__SSSE3__)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
elseif(APPLE)
    set(BACKEND RS_USE_LIBUVC_BACKEND)
else()
    set(BACKEND RS_USE_V4L2_BACKEND)
endif()
add_definitions(-D${BACKEND} -DUNICODE)

if(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(LIBUSB1 REQUIRED libusb-1.0)
    if(LIBUSB1_FOUND)
      include_directories(SYSTEM ${LIBUSB1_INCLUDE_DIRS})
    else()
      message( FATAL_ERROR "Failed to find libusb-1.0" )
    endif(LIBUSB1_FOUND)

    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fPIC -pedantic -D_BSD_SOURCE")
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

option(BUILD_SHARED_LIBS "Build shared library" ON)
if(BUILD_SHARED_LIBS)
    add_library(realsense SHARED ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_DEF})
    target_link_libraries(realsense ${LIBUSB1_LIBRARIES})
else()
    add_library(realsense STATIC ${REALSENSE_CPP} ${REALSENSE_HPP})
endif()

set_target_properties (realsense PROPERTIES
    FOLDER Library
)

target_include_directories(realsense PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${LIBUSB1_INCLUDE_DIRS})

install( TARGETS realsense
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/librealsense DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

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
