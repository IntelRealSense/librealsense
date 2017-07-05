// See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "../include/rs4xx_advanced_mode.h"
#include <stdlib.h>
#include <stdio.h>

/* Function calls to librealsense may raise errors of type rs2_error */
rs2_error * e = 0;
void check_error()
{
    if (e)
    {
        printf("rs2_error was raised when calling %s(%s):\n", rs2_get_failed_function(e), rs2_get_failed_args(e));
        printf("    %s\n", rs2_get_error_message(e));
        exit(EXIT_FAILURE);
    }
}

int main()
{
    /* Create a context object. This object owns the handles to all connected realsense devices. */
    rs2_context * ctx = rs2_create_context(RS2_API_VERSION, &e);
    check_error();

    rs2_device_list * devices = rs2_query_devices(ctx, &e);
    printf("There are %d connected RealSense devices.\n", rs2_get_device_count(devices, &e));
    check_error();
    if (rs2_get_device_count(devices, &e) == 0) return EXIT_FAILURE;

    /* This tutorial will access only a single device, but it is trivial to extend to multiple devices */
    rs2_device * dev = rs2_create_device(devices, 0, &e);
    check_error();

    STDepthControlGroup depth_control;

    printf("Reading deepSeaMedianThreshold from Depth Control...\n");
    int result = get_depth_control(dev, &depth_control);
    if (result)
    {
        printf("Advanced mode get failed!\n");
        return EXIT_FAILURE;
    }

    printf("deepSeaMedianThreshold = %d\n", depth_control.deepSeaMedianThreshold);

    printf("Writing Depth Control back to the device...\n");
    set_depth_control(dev, &depth_control);
    if (result)
    {
        printf("Advanced mode set failed!\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

