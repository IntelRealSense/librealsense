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
    file(STRINGS "./include/librealsense/rs2.h" REALSENSE_VERSION_${VER_COMPONENT} REGEX "#define RS2_API_${VER_COMPONENT}_VERSION")
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
    src/subdevice.cpp
    src/algo.cpp
    src/sync.cpp
    src/option.cpp
    src/error-handling.cpp
    src/hw-monitor.cpp
    src/image.cpp
    src/ivcam-private.cpp
    src/log.cpp
    src/rs.cpp
    src/sr300.cpp
    src/types.cpp
    src/backend-v4l2.cpp
    src/backend-hid.cpp
    src/backend.cpp
    src/verify.c
    src/ds5.cpp
    src/ds5-options.cpp
    src/ds5-timestamp.cpp
    src/ds5-private.cpp
    src/win/win-helpers.cpp
    src/win/win-uvc.cpp
    src/win/win-usb.cpp
    src/win/win-hid.cpp
    src/win/win-backend.cpp

    third_party/easyloggingpp/src/easylogging++.cc

    third_party/sqlite/sqlite3.c
    src/sql.cpp
    src/recorder.cpp

)

set(REALSENSE_HPP
    include/librealsense/rs2.hpp
    include/librealsense/rs2.h
    include/librealsense/rscore2.hpp
    include/librealsense/rsutil2.h
    include/librealsense/rsutil2.hpp

    src/archive.h
    src/concurrency.h
    src/context.h
    src/device.h
    src/sync.h
    src/subdevice.h
    src/algo.h
    src/option.h
    src/error-handling.h
    src/hw-monitor.h
    src/image.h
    src/ivcam-private.h
    src/types.h
    src/backend.h
    src/sr300.h
    src/ds5.h
    src/ds5-options.h
    src/ds5-private.h
    src/ds5-timestamp.h
    src/backend-v4l2.h
    src/backend-hid.h
    src/win/win-helpers.h
    src/win/win-uvc.h
    src/win/win-usb.h
    src/win/win-hid.h
    src/win/win-backend.h

    third_party/easyloggingpp/src/easylogging++.h
    third_party/sqlite/sqlite3.h
    src/sql.h
    src/recorder.h
)

if(WIN32)
    source_group("Source Files\\Backend" FILES
        src/win/win-helpers.cpp
        src/win/win-uvc.cpp
        src/win/win-usb.cpp
        src/win/win-hid.cpp
        src/win/win-backend.cpp
        src/backend-v4l2.cpp
        src/backend-hid.cpp
        src/backend.cpp
        src/verify.c)

    source_group("Header Files\\Backend" FILES
        src/win/win-helpers.h
        src/win/win-uvc.h
        src/win/win-usb.h
        src/win/win-hid.h
        src/win/win-backend.h
        src/backend-v4l2.h
        src/backend-hid.h
        src/backend.h)

    source_group("Header Files\\API" FILES
        include/librealsense/rs2.hpp
        include/librealsense/rs2.h
        include/librealsense/rscore2.hpp
        include/librealsense/rsutil2.h
        include/librealsense/rsutil2.hpp
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
        src/ds5-options.cpp
        src/ds5-private.cpp
        src/ds5-timestamp.cpp
        src/sr300.cpp
        src/ivcam-private.cpp
        )

    source_group("Header Files\\Devices" FILES
        src/ds5.h
        src/ds5-options.h
        src/ds5-private.h
        src/ds5-timestamp.h
        src/sr300.h
        src/ivcam-private.h
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

if(WIN32)
    set(BACKEND RS2_USE_WMF_BACKEND)
    set(REALSENSE_DEF CMake/realsense.def)
    # Makes VS15 find the DLL when trying to run examples/tests
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    add_definitions(-D__SSSE3__ -D_CRT_SECURE_NO_WARNINGS)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
elseif(APPLE)
    set(BACKEND RS2_USE_LIBUVC_BACKEND)
else()
    set(BACKEND RS2_USE_V4L2_BACKEND)
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
    find_package(PkgConfig REQUIRED)
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

option(BUILD_SHARED_LIBS "Build shared library" ON)
if(BUILD_SHARED_LIBS)
    add_library(realsense2 SHARED ${REALSENSE_CPP} ${REALSENSE_HPP} ${REALSENSE_DEF})
    set_target_properties(realsense2 PROPERTIES VERSION ${REALSENSE_VERSION_STRING}
                                    SOVERSION ${REALSENSE_VERSION_MAJOR})
    target_link_libraries(realsense2 ${LIBUSB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    list(APPEND librealsense_PKG_LIBS ${CMAKE_THREAD_LIBS_INIT})
else()
    add_library(realsense2 STATIC ${REALSENSE_CPP} ${REALSENSE_HPP})
endif()

add_definitions(-DELPP_THREAD_SAFE)
add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)


set_target_properties (realsense2 PROPERTIES
    FOLDER Library
)

set(LRS_LIB_NAME realsense)

target_include_directories(realsense2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/ ${LIBUSB1_INCLUDE_DIRS})

IF (${ROS_BUILD_TYPE})
    install(TARGETS realsense
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
    )

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
ELSE()
    install( TARGETS realsense2
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/librealsense DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(CODE "execute_process(COMMAND ldconfig)")
ENDIF()

option(BUILD_EXAMPLES "Build realsense examples." OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

option(BUILD_UNIT_TESTS "Build realsense unit tests." ON)
if(BUILD_UNIT_TESTS)
  add_subdirectory(unit-tests)
endif()

add_subdirectory(bindings)

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
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/config/realsense2.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
