target_sources(${LRS_TARGET}
    PRIVATE
		"${CMAKE_CURRENT_LIST_DIR}/libusb/core.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/descriptor.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/hotplug.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/io.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/strerror.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/sync.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/linux_usbfs.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/poll_posix.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/threads_posix.c"
		"${CMAKE_CURRENT_LIST_DIR}/libusb/linux_netlink.c"
)

