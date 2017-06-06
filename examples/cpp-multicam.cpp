// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"

#include <iostream>
#include <algorithm>
#include <sstream>


using namespace rs2;
using namespace std;

/***************

***************/

int main(int argc, char * argv[]) try
{
    log_to_console(RS2_LOG_SEVERITY_WARN);

    vector<texture_buffer> buffers;

    context ctx; // Create librealsense context
    auto devices = ctx.query_devices(); // Query the list of connected RealSense devices
    if (devices.size() == 0)
    {
        throw runtime_error("No device detected. Is it plugged in?");
    }

    vector<frame_queue> syncers;

    // Configure and start our devices
    for(auto&& dev : devices)
    {
        cout << "Starting " << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME) << "... ";
        auto modes = dev.get_stream_modes();
        dev.open(modes.back());
        syncers.emplace_back();
        dev.start(syncers.back());
        cout << "done." << endl;
    }

    // Depth and color
    buffers.resize(devices.size());

    // Open a GLFW window
    glfwInit();
    ostringstream ss; ss << "CPP Multi-Camera Example";
    auto win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    int windowWidth, windowHeight;
    glfwGetWindowSize(win, &windowWidth, &windowHeight);

    // Does not account for correct aspect ratios
    auto perTextureWidth = 2 * static_cast<float>(windowWidth / devices.size());
    auto perTextureHeight = 480.0f;

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();

        for(int i = 0; i < syncers.size(); i++)
        {
            frame frame;
            if (syncers[i].poll_for_frame(&frame))
            {
                buffers[i].upload(frame);
            }
        }

        // Draw the images
        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(win, &w, &h);
        glPushMatrix();
        glOrtho(0, w, h, 0, -1, +1);
        glPixelZoom(1, -1);

        for(int i = 0; i < syncers.size(); i++)
        {
            buffers[i].show({ (i / 2) * perTextureWidth, (i % 2) * perTextureHeight, perTextureWidth, perTextureHeight }, 1);
        }

        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();

    buffers.clear();

    return EXIT_SUCCESS;
}
catch(const error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
catch(const exception & e)
{
    cerr << e.what() << endl;
    return EXIT_FAILURE;
}
