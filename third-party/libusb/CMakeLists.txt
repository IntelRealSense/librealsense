# ubuntu 12.04 LTS cmake version 2.8.7
# ubuntu 14.04 LTS cmake version 2.8.12.2
# ubuntu 16.04 LTS cmake version 3.5.1
cmake_minimum_required(VERSION 2.8.3)

project(usb)

set(LIBUSB_C
  libusb/descriptor.c
  libusb/core.c
  libusb/hotplug.c
  libusb/io.c
  libusb/sync.c
  libusb/strerror.c
  libusb/os/linux_usbfs.c
  libusb/os/poll_posix.c
  libusb/os/threads_posix.c
  libusb/os/linux_netlink.c
)

set(LIBUSB_H
    libusb/libusb.h
)

include_directories(
  libusb/libusb
  libusb/libusb/os
)

add_library(usb STATIC ${LIBUSB_C} ${LIBUSB_H})


install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    )
