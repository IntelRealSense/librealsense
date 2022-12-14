// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

// GCC, when using -pedantic, gives the following inside libusb.h:
#if 0
In file included from /home/administrator/lrs/src/libusb/request-libusb.h:6:0,
    from /home/administrator/lrs/src/libusb/request-libusb.cpp:4:
/usr/include/libusb-1.0/libusb.h:736:4: warning: ISO C++ forbids zero-size array 'dev_capability_data' [-Wpedantic]
    [0] /* non-standard, but usually working code */
    ^
/usr/include/libusb-1.0/libusb.h:767:4: warning: ISO C++ forbids zero-size array 'dev_capability' [-Wpedantic]
    [0] /* non-standard, but usually working code */
    ^
In file included from /home/administrator/lrs/src/libusb/request-libusb.h:6:0,
    from /home/administrator/lrs/src/libusb/request-libusb.cpp:4:
/usr/include/libusb-1.0/libusb.h:1260:4: warning: ISO C++ forbids zero-size array 'iso_packet_desc' [-Wpedantic]
    [0] /* non-standard, but usually working code */
    ^
#endif


#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#include <libusb.h>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

