/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs_net.h
* \
* Exposes RealSense network device functionality for C compilers
*/

#ifndef LIBREALSENSE_RS2_NET_H
#define LIBREALSENSE_RS2_NET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "librealsense2/rs.h"

/**
* create RealSense net device by ip address
*/
rs2_device* rs2_create_net_device(int api_version, const char* address, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif
