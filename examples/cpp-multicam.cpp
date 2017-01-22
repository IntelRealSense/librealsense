// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include "example.hpp"

#include <iostream>
#include <algorithm>
#include <sstream>

std::vector<texture_buffer> buffers;

/***************

***************/

int main(int argc, char * argv[]) try
{
    rs::log_to_console(RS_LOG_SEVERITY_WARN);

    rs::context ctx; // Create librealsense context
    auto devices = ctx.query_devices(); // Query the list of connected RealSense devices
    if (devices.size() == 0)
    {
        throw std::runtime_error("No device detected. Is it plugged in?");
    }

    std::vector<rs::frame_queue> syncers;

    // Configure and start our devices
    for(auto&& dev : devices)
    {
        std::cout << "Starting " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << "... ";
        auto modes = dev.get_stream_modes();
        dev.open(modes.back());
        syncers.emplace_back();
        dev.start(syncers.back());
        std::cout << "done." << std::endl;
    }

    // Depth and color
    buffers.resize(devices.size());

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Multi-Camera Example";
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
            rs::frame frame;
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
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
