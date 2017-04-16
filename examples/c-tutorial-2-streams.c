/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

/*******************************************************\
* librealsense tutorial #2 - Accessing multiple streams *
\*******************************************************/

/* First include the librealsense C header file */
#include <librealsense/rs2.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* Also include GLFW to allow for graphical display */
#include <GLFW/glfw3.h>

/* Function calls to librealsense may raise errors of type rs2_error */
rs2_error * e = 0;
void check_error()
{
    if(e)
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
    if(rs2_get_device_count(devices, &e) == 0)
        return EXIT_FAILURE;

    /* This tutorial will access only a single device, but it is trivial to extend to multiple devices */
    rs2_device * dev = rs2_create_device(devices, 0, &e);
    check_error();
    printf("\nUsing device 0, an %s\n", rs2_get_camera_info(dev, RS2_CAMERA_INFO_DEVICE_NAME, &e));
    check_error();
    printf("    Serial number: %s\n", rs2_get_camera_info(dev, RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER, &e));
    check_error();
    printf("    Firmware version: %s\n", rs2_get_camera_info(dev, RS2_CAMERA_INFO_CAMERA_FIRMWARE_VERSION, &e));
    check_error();
    printf("    Device Location: %s\n", rs2_get_camera_info(dev, RS2_CAMERA_INFO_DEVICE_LOCATION, &e));
    check_error();

    rs2_stream streams[] = { RS2_STREAM_DEPTH, RS2_STREAM_INFRARED };
    int widths[] = { 640, 640 };
    int heights[] = { 480, 480 };
    int fpss[] = { 30, 30 };
    rs2_format formats[] = { RS2_FORMAT_Z16, RS2_FORMAT_Y8 };

    rs2_open_multiple(dev, streams, widths, heights, fpss, formats, 2, &e);
    check_error();

    rs2_frame_queue * queue = rs2_create_frame_queue(10, &e);
    check_error();

    rs2_start(dev, rs2_enqueue_frame, queue, &e);
    check_error();

    rs2_frame* frontbuffer[RS2_STREAM_COUNT];
    for (int i = 0; i < RS2_STREAM_COUNT; i++)
        frontbuffer[i] = NULL;

    float depth_units = 1.f;
    if (rs2_supports_option(dev, RS2_OPTION_DEPTH_UNITS, &e) && !e)
    {
        depth_units = rs2_get_option(dev, RS2_OPTION_DEPTH_UNITS, &e);
        check_error();
    }

    /* Open a GLFW window to display our output */
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(640, 480 * 2, "librealsense tutorial #2", NULL, NULL);
    glfwMakeContextCurrent(win);
    while(!glfwWindowShouldClose(win))
    {
        /* Wait for new frame data */
        glfwPollEvents();
        rs2_frame* frame;

        while (rs2_poll_for_frame(queue, &frame, &e))
        {
            check_error();

            rs2_stream stream_type = rs2_get_frame_stream_type(frame, &e);
            check_error();

            if (frontbuffer[stream_type])
            {
                rs2_release_frame(frontbuffer[stream_type]);
            }
            frontbuffer[stream_type] = frame;
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glPixelZoom(1, -1);

        if (frontbuffer[RS2_STREAM_DEPTH])
        {
            /* Display depth data by linearly mapping depth between 0 and 2 meters to the red channel */
            glRasterPos2f(-1, 1);
            glPixelTransferf(GL_RED_SCALE, (0xFFFF * depth_units) / 2.0f);
            check_error();
            glDrawPixels(640, 480, GL_RED, GL_UNSIGNED_SHORT, rs2_get_frame_data(frontbuffer[RS2_STREAM_DEPTH], &e));
            check_error();
            glPixelTransferf(GL_RED_SCALE, 1.0f);
        }

        if (frontbuffer[RS2_STREAM_INFRARED])
        {
            /* Display infrared image by mapping IR intensity to visible luminance */
            glRasterPos2f(-1, 0);
            glDrawPixels(640, 480, GL_LUMINANCE, GL_UNSIGNED_BYTE, rs2_get_frame_data(frontbuffer[RS2_STREAM_INFRARED], &e));
            check_error();
        }

        glfwSwapBuffers(win);
    }

    for (int i = 0; i < RS2_STREAM_COUNT; i++)
    {
        if (frontbuffer[i])
        {
            rs2_release_frame(frontbuffer[i]);
        }
    }

    rs2_flush_queue(queue, &e);
    check_error();
    rs2_close(dev, &e);
    check_error();
    rs2_delete_device(dev);
    rs2_delete_device_list(devices);
    rs2_delete_frame_queue(queue);
    rs2_delete_context(ctx);

    return EXIT_SUCCESS;
}
