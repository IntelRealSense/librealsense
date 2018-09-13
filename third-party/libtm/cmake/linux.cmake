set(OS "lin")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

include_directories("/usr/include/libusb-1.0")
set(OS_SPECIFIC_LIBS "pthread")
set(LIBUSB_LIB "usb-1.0")
