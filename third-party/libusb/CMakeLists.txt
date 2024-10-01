# our current cmake version is 3.8
cmake_minimum_required(VERSION 3.8)

project(usb)

set (POSIX_PLATFORM_SRC
libusb/os/events_posix.c
libusb/os/threads_posix.c
)

set (WINDOWS_PLATFORM_SRC
libusb/os/events_windows.c
libusb/os/threads_windows.c
)

if(UNIX)
  message(STATUS "LibUSB - POSIX system detected.")
  set(PLATFORM_SRC "${POSIX_PLATFORM_SRC}")
else()
  message(STATUS "LibUSB - Non-POSIX system detected.")
  set(PLATFORM_SRC "${WINDOWS_PLATFORM_SRC}")
endif()

set(LIBUSB_C
libusb/core.c
libusb/descriptor.c
libusb/hotplug.c
libusb/io.c
libusb/strerror.c
libusb/sync.c
)

LIST(APPEND LIBUSB_C 
  ${PLATFORM_SRC}
  )

if(WIN32)
  LIST(APPEND LIBUSB_C
  libusb/os/windows_common.c
  libusb/os/windows_usbdk.c
  libusb/os/windows_winusb.c
  )
elseif (APPLE)
  LIST(APPEND LIBUSB_C
  libusb/os/darwin_usb.c
  
  )
elseif(ANDROID)
  LIST(APPEND LIBUSB_C
  libusb/os/linux_usbfs.c
  libusb/os/linux_netlink.c
  )
else()
  LIST(APPEND LIBUSB_C
  libusb/os/linux_usbfs.c
  libusb/os/linux_udev.c
  )
endif()

set(LIBUSB_H
    libusb/libusb.h
)

include_directories(
  libusb
  libusb/os
)

add_library(usb STATIC ${LIBUSB_C} ${LIBUSB_H})

if(WIN32)
    set_target_properties (usb PROPERTIES
        FOLDER "3rd Party"
    )
    include_directories(msvc)
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /utf-8")
endif()

if(ANDROID)
    include_directories(android)
endif()

if(APPLE)
  find_library(corefoundation_lib CoreFoundation)
  find_library(iokit_lib IOKit)
  target_include_directories(usb PRIVATE XCode)
  TARGET_LINK_LIBRARIES(usb objc ${corefoundation_lib} ${iokit_lib})
endif()

if((NOT APPLE) AND (NOT ANDROID) AND (NOT WIN32))
  TARGET_LINK_LIBRARIES(usb udev)
endif()

#set_target_properties(usb PROPERTIES PREFIX "")

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )
