set(OS "win")

set(WINDOWS_LIBUSB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third-party/win/libusb/include")

message("Platform is set to ${PLATFORM}")

set(WINDOWS_LIBUSB_LIB_DIR "${CMAKE_SOURCE_DIR}/third-party/win/libusb/bin/x64")

include_directories("${WINDOWS_LIBUSB_INCLUDE_DIR}")

set(LIBUSB_LIB "${WINDOWS_LIBUSB_LIB_DIR}/libusb-1.0.lib")
