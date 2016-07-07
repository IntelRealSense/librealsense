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
    printf("There are %d connected RealSense devices.\n", rs_get_device_count(ctx, &e));
    check_error();
    if(rs_get_device_count(ctx, &e) == 0) return EXIT_FAILURE;

    /* This tutorial will access only a single device, but it is trivial to extend to multiple devices */
    rs_device * dev = rs_get_device(ctx, 0, &e);
    check_error();
    printf("\nUsing device 0, an %s\n", rs_get_device_name(dev, &e));
    check_error();
    printf("    Serial number: %s\n", rs_get_device_serial(dev, &e));
    check_error();
    printf("    Firmware version: %s\n", rs_get_device_firmware_version(dev, &e));
    check_error();

    /* Configure all streams to run at VGA resolution at 60 frames per second */
    rs_enable_stream(dev, RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60, &e);
    check_error();
    rs_enable_stream(dev, RS_STREAM_COLOR, 640, 480, RS_FORMAT_RGB8, 60, &e);
    check_error();
    rs_enable_stream(dev, RS_STREAM_INFRARED, 640, 480, RS_FORMAT_Y8, 60, &e);
    check_error();
    rs_enable_stream(dev, RS_STREAM_INFRARED2, 640, 480, RS_FORMAT_Y8, 60, NULL); /* Pass NULL to ignore errors */
    rs_start_device(dev, &e);
    check_error();

    /* Open a GLFW window to display our output */
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 960, "librealsense tutorial #2", NULL, NULL);
    glfwMakeContextCurrent(win);
    while(!glfwWindowShouldClose(win))
    {
        /* Wait for new frame data */
        glfwPollEvents();
        rs_wait_for_frames(dev, &e);
        check_error();

        glClear(GL_COLOR_BUFFER_BIT);
        glPixelZoom(1, -1);

        /* Display depth data by linearly mapping depth between 0 and 2 meters to the red channel */
        glRasterPos2f(-1, 1);
        glPixelTransferf(GL_RED_SCALE, 0xFFFF * rs_get_device_depth_scale(dev, &e) / 2.0f);
        check_error();
        glDrawPixels(640, 480, GL_RED, GL_UNSIGNED_SHORT, rs_get_frame_data(dev, RS_STREAM_DEPTH, &e));
        check_error();
        glPixelTransferf(GL_RED_SCALE, 1.0f);

        /* Display color image as RGB triples */
        glRasterPos2f(0, 1);
        glDrawPixels(640, 480, GL_RGB, GL_UNSIGNED_BYTE, rs_get_frame_data(dev, RS_STREAM_COLOR, &e));
        check_error();

        /* Display infrared image by mapping IR intensity to visible luminance */
        glRasterPos2f(-1, 0);
        glDrawPixels(640, 480, GL_LUMINANCE, GL_UNSIGNED_BYTE, rs_get_frame_data(dev, RS_STREAM_INFRARED, &e));
        check_error();

        /* Display second infrared image by mapping IR intensity to visible luminance */
        if(rs_is_stream_enabled(dev, RS_STREAM_INFRARED2, NULL))
        {
            glRasterPos2f(0, 0);
            glDrawPixels(640, 480, GL_LUMINANCE, GL_UNSIGNED_BYTE, rs_get_frame_data(dev, RS_STREAM_INFRARED2, &e));
        }

        glfwSwapBuffers(win);
    }
    
    return EXIT_SUCCESS;
}
