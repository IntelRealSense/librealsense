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
    rs_device_list * devices = rs_query_devices(ctx, &e);
    printf("There are %d connected RealSense devices.\n", rs_get_device_count(devices, &e));
    check_error();
    if (rs_get_device_count(devices, &e) == 0)
        return EXIT_FAILURE;

    /* This tutorial will access only a single device, but it is trivial to extend to multiple devices */
    rs_device * dev = rs_create_device(devices, 0, &e);
    check_error();
    printf("\nUsing device 0, an %s\n", rs_get_camera_info(dev, RS_CAMERA_INFO_DEVICE_NAME, &e));
    check_error();
    printf("    Serial number: %s\n", rs_get_camera_info(dev, RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER, &e));
    check_error();
    printf("    Firmware version: %s\n", rs_get_camera_info(dev, RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION, &e));
    check_error();
    printf("    Device Location: %s\n", rs_get_camera_info(dev, RS_CAMERA_INFO_DEVICE_LOCATION, &e));
    check_error();

    /* Configure depth to run at VGA resolution at 30 frames per second */
    int width = 640, height = 480;
    rs_open(dev, RS_STREAM_DEPTH, width, height, 30, RS_FORMAT_Z16, &e);
    check_error();

    rs_frame_queue * queue = rs_create_frame_queue(1, &e);
    check_error();
    rs_start(dev, rs_enqueue_frame, queue, &e);
    check_error();

    /* Determine depth value corresponding to one meter */
    const uint16_t one_meter = (uint16_t)(1.0f / rs_get_device_depth_scale(dev, &e));
    check_error();

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
        rs_frame * frame = rs_wait_for_frame(queue, &e);
        check_error();

        /* Retrieve depth data, configured as 16-bit depth values */
        const uint16_t * depth_frame = (const uint16_t *)(rs_get_frame_data(frame, &e));
        check_error();

        /* Print a simple text-based representation of the image, by breaking it into 10x5 pixel regions and and approximating the coverage of pixels within one meter */
        out = buffer;
        int coverage[255] = { 0 }, x, y, i;
        for (y = 0; y < height; ++y)
        {
            for (x = 0; x < width; ++x)
            {
                int depth = *depth_frame++;
                if (depth > 0 && depth < one_meter)
                    ++coverage[x / 10];
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

        rs_release_frame(frame);
    }

    rs_flush_queue(queue, &e);
    check_error();

    rs_stop(dev, &e);
    check_error();

    rs_close(dev, &e);
    check_error();

    rs_delete_device(dev);
    rs_delete_frame_queue(queue);
    rs_delete_device_list(devices);

    free(buffer);

    return EXIT_SUCCESS;
}
