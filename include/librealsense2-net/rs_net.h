/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

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
 * Net device is a rs2_device that can be stream and be contolled remotely over network 
 * \param[in] api_version Users are expected to pass their version of \c RS2_API_VERSION to make sure they are running the correct librealsense version.
 * \param[in] address remote devce ip address. should be the address of the hosting device
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
rs2_device* rs2_create_net_device(int api_version, const char* address, rs2_error** error);

/**
* Net server
*/
typedef void rs_server;

typedef struct
{
    char* iface; // interface to bind - NULL for the default ("0.0.0.0")
    int   port;  // port to listen    - zero for the default (8554)
    int   flags; // mode of work, etc
} rs_server_params;

rs_server* rs2_create_server(int api_version, rs2_device* dev, rs_server_params params, rs2_error** error);
int rs2_start_server(int api_version, rs_server* srv, rs2_error** error);
int rs2_stop_server(int api_version, rs_server* srv, rs2_error** error);
int rs2_destroy_server(int api_version, rs_server* srv, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif
