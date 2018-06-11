#  minimum required cmake version: 3.8.0
cmake_minimum_required(VERSION 3.8.0)

project(usb)

  set(LIBUSB_C
  libusb/core.c
  libusb/descriptor.c
  libusb/hotplug.c
  libusb/io.c
  libusb/strerror.c
  libusb/sync.c
  )

if(WIN32)
  LIST(APPEND LIBUSB_C
  libusb/os/threads_windows.c
  libusb/os/poll_windows.c
  libusb/os/windows_winusb.c
  libusb/os/windows_nt_common.c
  )
elseif (APPLE)
  LIST(APPEND LIBUSB_C
  libusb/os/poll_posix.c
  libusb/os/threads_posix.c
  libusb/os/darwin_usb.c
  )
elseif(ANDROID)
  LIST(APPEND LIBUSB_C
  libusb/os/linux_usbfs.c
  libusb/os/poll_posix.c
  libusb/os/threads_posix.c
  libusb/os/linux_netlink.c
  )
else()
  LIST(APPEND LIBUSB_C
  libusb/os/linux_usbfs.c
  libusb/os/poll_posix.c
  libusb/os/threads_posix.c
  libusb/os/linux_udev.c
  )
endif()

set(LIBUSB_H
    libusb/libusb.h
)

include_directories(
  libusb/libusb
  libusb/libusb/os
)

add_library(usb STATIC ${LIBUSB_C} ${LIBUSB_H})

if(WIN32)
    set_target_properties (usb PROPERTIES
        FOLDER "3rd Party"
    )
endif()

if(APPLE)
  find_library(corefoundation_lib CoreFoundation)
  find_library(iokit_lib IOKit)
  TARGET_LINK_LIBRARIES(usb objc ${corefoundation_lib} ${iokit_lib})
endif()

if((NOT APPLE) AND (NOT ANDROID) AND (NOT WIN32))
  TARGET_LINK_LIBRARIES(usb udev)
endif()

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )
