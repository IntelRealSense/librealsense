/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

/*************************************************\
* librealsense tutorial #1 - Accessing depth data *
\*************************************************/

/* First include the librealsense C header file */
#include <librealsense/rs.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* Function calls to librealsense may raise errors of type rs_error */
rs_error * e = 0;
void check_error()
{
    if (e)
    {
        printf("rs_error was raised when calling %s(%s):\n", rs_get_failed_function(e), rs_get_failed_args(e));
        printf("    %s\n", rs_get_error_message(e));
        exit(EXIT_FAILURE);
    }
}

int main()
{
    /* Create a context object. This object owns the handles to all connected realsense devices. */
    rs_context * ctx = rs_create_context(RS_API_VERSION, &e);
    check_error();
    printf("There are %d connected RealSense devices.\n", rs_get_device_count(ctx, &e));
    check_error();
    if (rs_get_device_count(ctx, &e) == 0) return EXIT_FAILURE;

    /* This tutorial will access only a single device, but it is trivial to extend to multiple devices */
    rs_device * dev = rs_get_device(ctx, 0, &e);
    check_error();
    printf("\nUsing device 0, an %s\n", rs_get_device_name(dev, &e));
    check_error();
    printf("    Serial number: %s\n", rs_get_device_serial(dev, &e));
    check_error();
    printf("    Firmware version: %s\n", rs_get_device_firmware_version(dev, &e));
    check_error();

    rs_stream stream_type = RS_STREAM_DEPTH;
    /* Configure depth to run at VGA resolution at 30 frames per second */
    rs_enable_stream(dev, RS_STREAM_DEPTH, 0, 0, RS_FORMAT_Z16, 30, &e);
    check_error();
    rs_start_device(dev, &e);
    check_error();

    /* Determine depth value corresponding to one meter */
    const uint16_t one_meter = (uint16_t)(1.0f / rs_get_device_depth_scale(dev, &e));
    check_error();

    /* retrieve actual frame size */
    rs_intrinsics depth_intrin;
    rs_get_stream_intrinsics(dev, stream_type, &depth_intrin, &e);
    check_error();
    int width = depth_intrin.width;
    int height = depth_intrin.height;

    int rows = (height / 20);
    int row_lenght = (width / 10);
    int display_size = (rows + 1) * (row_lenght + 1);

    char *buffer = (char*)malloc(display_size * sizeof(char));
    if (NULL == buffer)
    {
        printf("Failed to allocate application memory");
        exit(EXIT_FAILURE);
    }
    char * out;

    while (1)
    {
        /* This call waits until a new coherent set of frames is available on a device */
        rs_wait_for_frames(dev, &e);

        /* Retrieve depth data, configured as 16-bit depth values */
        const uint16_t * depth_frame = (const uint16_t *)(rs_get_frame_data(dev, RS_STREAM_DEPTH, &e));

        /* Print a simple text-based representation of the image, by breaking it into 10x5 pixel regions and and approximating the coverage of pixels within one meter */
        out = buffer;
        int coverage[255] = { 0 }, x, y, i;
        for (y = 0; y < height; ++y)
        {
            for (x = 0; x < width; ++x)
            {
                int depth = *depth_frame++;
                if (depth > 0 && depth < one_meter) ++coverage[x / 10];
            }

            if (y % 20 == 19)
            {
                for (i = 0; i < (row_lenght); ++i)
                {
                    *out++ = " .:nhBXWW"[coverage[i] / 25];
                    coverage[i] = 0;
                }
                *out++ = '\n';
            }
        }
        *out++ = 0;
        printf("\n%s", buffer);
    }

    rs_stop_device(dev, &e);
    check_error();

    free(buffer);

    return EXIT_SUCCESS;
}
