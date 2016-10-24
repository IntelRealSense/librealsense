/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2015 Intel Corporation. All Rights Reserved. */

/*******************************************************\
* librealsense tutorial #2 - Accessing multiple streams *
\*******************************************************/

/* First include the librealsense C header file */
#include <librealsense/rs.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

/* Also include GLFW to allow for graphical display */
#include <GLFW/glfw3.h>

/* Function calls to librealsense may raise errors of type rs_error */
rs_error * e = 0;
void check_error()
{
    if(e)
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
    if(rs_get_device_count(devices, &e) == 0) return EXIT_FAILURE;

    /* This tutorial will access only a single device, but it is trivial to extend to multiple devices */
    rs_device * dev = rs_create_device(devices, 1, &e);
    check_error();
    printf("\nUsing device 0, an %s\n", rs_get_camera_info(dev, RS_CAMERA_INFO_DEVICE_NAME, &e));
    check_error();
    printf("    Serial number: %s\n", rs_get_camera_info(dev, RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER, &e));
    check_error();
    printf("    Firmware version: %s\n", rs_get_camera_info(dev, RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION, &e));
    check_error();

    rs_stream streams[3] = { RS_STREAM_COLOR, RS_STREAM_DEPTH, RS_STREAM_INFRARED };
    int widths[3] = { 640, 640, 640 };
    int heights[3] = { 480, 480, 480 };
    int fpss[3] = { 30, 30, 30 };
    rs_format formats[3] = { RS_FORMAT_BGR8, RS_FORMAT_Z16, RS_FORMAT_Y8 };

    rs_active_stream * color_stream = rs_open_many(dev, RS_SUBDEVICE_COLOR, streams, widths, heights, fpss, formats, 1, &e);
    check_error();
    rs_active_stream * depth_stream = rs_open_many(dev, RS_SUBDEVICE_DEPTH, streams+1, widths+1, heights+1, fpss+1, formats+1, 2, &e);
    check_error();

    rs_frame_queue * queue = rs_create_frame_queue(10, &e);
    check_error();

    rs_start(color_stream, rs_enqueue_frame, queue, &e);
    check_error();
    rs_start(depth_stream, rs_enqueue_frame, queue, &e);
    check_error();

    rs_frame* frontbuffer[RS_STREAM_COUNT];
    rs_active_stream* owners[RS_STREAM_COUNT];
    for (int i = 0; i < RS_STREAM_COUNT; i++) frontbuffer[i] = NULL;

    /* Open a GLFW window to display our output */
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 960, "librealsense tutorial #2", NULL, NULL);
    glfwMakeContextCurrent(win);
    while(!glfwWindowShouldClose(win))
    {
        /* Wait for new frame data */
        glfwPollEvents();
        rs_active_stream * owner;
        rs_frame* frame = rs_wait_for_frame(queue, &owner, &e);
        check_error();

        rs_stream stream_type = rs_get_frame_stream_type(frame, &e);
        check_error();
        if (frontbuffer[stream_type])
        {
            rs_release_frame(owners[stream_type], frontbuffer[stream_type]);
        }
        frontbuffer[stream_type] = frame;
        owners[stream_type] = owner;

        glClear(GL_COLOR_BUFFER_BIT);
        glPixelZoom(1, -1);

        if (frontbuffer[RS_STREAM_DEPTH])
        {
            /* Display depth data by linearly mapping depth between 0 and 2 meters to the red channel */
            glRasterPos2f(-1, 1);
            glPixelTransferf(GL_RED_SCALE, 0xFFFF * rs_get_device_depth_scale(dev, &e) / 2.0f);
            check_error();
            glDrawPixels(640, 480, GL_RED, GL_UNSIGNED_SHORT, rs_get_frame_data(frontbuffer[RS_STREAM_DEPTH], &e));
            check_error();
            glPixelTransferf(GL_RED_SCALE, 1.0f);
        }

        if (frontbuffer[RS_STREAM_COLOR])
        {
            /* Display color image as RGB triples */
            glRasterPos2f(0, 1);
            glDrawPixels(640, 480, GL_RGB, GL_UNSIGNED_BYTE, rs_get_frame_data(frontbuffer[RS_STREAM_COLOR], &e));
            check_error();
        }

        if (frontbuffer[RS_STREAM_INFRARED])
        {
            /* Display infrared image by mapping IR intensity to visible luminance */
            glRasterPos2f(-1, 0);
            glDrawPixels(640, 480, GL_LUMINANCE, GL_UNSIGNED_BYTE, rs_get_frame_data(frontbuffer[RS_STREAM_INFRARED], &e));
            check_error();
        }

        glfwSwapBuffers(win);
    }

    for (int i = 0; i < RS_STREAM_COUNT; i++)
    {
        if (frontbuffer[i])
        {
            rs_release_frame(owners[i], frontbuffer[i]);
        }
    }

    rs_flush_queue(queue, &e);
    check_error();
    rs_close(color_stream);
    rs_close(depth_stream);
    rs_delete_device(dev);
    rs_delete_device_list(devices);
    rs_delete_frame_queue(queue);
    rs_delete_context(ctx);
    
    return EXIT_SUCCESS;
}
