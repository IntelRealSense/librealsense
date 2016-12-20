// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

///////////////////////////////////////////////////////////
// librealsense tutorial #2 - Accessing multiple streams //
///////////////////////////////////////////////////////////

// First include the librealsense C++ header file
#include <librealsense/rs.hpp>
#include <cstdio>

// Also include GLFW to allow for graphical display
#include <GLFW/glfw3.h>

int main() try
{
    // Create a context object. This object owns the handles to all connected realsense devices.
    rs::context ctx;
    auto connected_devices = ctx.query_devices();
    printf("There are %llu connected RealSense devices.\n", connected_devices.size());
    if(connected_devices.size() == 0) return EXIT_FAILURE;

    // This tutorial will access only a single device, but it is trivial to extend to multiple devices
    auto dev = connected_devices[0];
    printf("\nUsing device 0, an %s\n", dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME));
    printf("    Serial number: %s\n", dev.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER));
    printf("    Firmware version: %s\n", dev.get_camera_info(RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION));

    std::vector<rs_stream> supported_streams = { RS_STREAM_DEPTH, RS_STREAM_INFRARED, RS_STREAM_COLOR };

    // Configure the relevant subdevices of the RealSense camera
    dev.open({ { RS_STREAM_DEPTH, 640, 480, 30, RS_FORMAT_Z16 },
               { RS_STREAM_INFRARED, 640, 480, 30, RS_FORMAT_Y8 } });

    // Create frame queue to pass new frames from the device to our application
    rs::frame_queue queue(RS_STREAM_COUNT);
    // Create a frame buffer to hold on to most up to date frame until a newer frame is available
    rs::frame frontbuffer[RS_STREAM_COUNT];

    // Start the physical devices and specify our frame queue as the target
    dev.start(queue);

    // Open a GLFW window to display our output
    glfwInit();
    auto win = glfwCreateWindow(640, 480 * 2, "librealsense tutorial #2", nullptr, nullptr);
    glfwMakeContextCurrent(win);

    while(!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        // Move new frames into the frame buffer
        rs::frame frame;
        while (queue.poll_for_frame(&frame))
        {
            auto stream_type = frame.get_stream_type();
            // Override the last frame with the new one
            frontbuffer[stream_type] = std::move(frame);
        }

        glClear(GL_COLOR_BUFFER_BIT);
        glPixelZoom(1, -1);

        for (auto & stream : supported_streams)
        {
            if (frontbuffer[stream])
            switch (stream)
            {
                case RS_STREAM_DEPTH: // Display depth data by linearly mapping depth between 0 and 2 meters to the red channel
                    glRasterPos2f(-1, 1);
                    glPixelTransferf(GL_RED_SCALE, 0xFFFF * dev.get_depth_scale() / 2.0f);
                    glDrawPixels(640, 480, GL_RED, GL_UNSIGNED_SHORT, frontbuffer[stream].get_data());
                    glPixelTransferf(GL_RED_SCALE, 1.0f);
                break;
                case RS_STREAM_INFRARED: // Display infrared image by mapping IR intensity to visible luminance
                    glRasterPos2f(-1, 0);
                    glDrawPixels(640, 480, GL_LUMINANCE, GL_UNSIGNED_BYTE, frontbuffer[stream].get_data());
                break;
                default: break;    // This demo will display native streams only
            }
        }

        glfwSwapBuffers(win);
    }

    printf("Finishing up");

    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    // Method calls against librealsense objects may throw exceptions of type rs::error
    printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
    printf("    %s\n", e.what());
    return EXIT_FAILURE;
}
