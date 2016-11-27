// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include "example.hpp"

#include <iostream>
#include <algorithm>
#include <sstream>

std::vector<texture_buffer> buffers;

int main(int argc, char * argv[]) try
{
    rs::log_to_console(RS_LOG_SEVERITY_WARN);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    auto devices = ctx.query_devices();
    if(devices.size() == 0) throw std::runtime_error("No device detected. Is it plugged in?");

    std::vector<rs::streaming_lock> streams;
    std::vector<rs::frame_queue> syncers;

    // Configure and start our devices
    for(auto&& dev : devices)
    {
        std::cout << "Starting " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << "... ";
        rs::util::config config;
        auto modes = dev.get_stream_modes();
        streams.push_back(dev.open(modes[0]));
        syncers.emplace_back();
        streams.back().start(syncers.back());
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
        
        // Draw the images
        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwGetWindowSize(win, &w, &h);
        glPushMatrix();
        glOrtho(0, w, h, 0, -1, +1);
        glPixelZoom(1, -1);
        auto i = 0;

        for(auto&& syncer : syncers)
        {
            rs::frame frame;
            if (syncer.poll_for_frame(&frame))
            {
                buffers[i].upload(frame);
            }
            buffers[i++].show({ (i / 2) * perTextureWidth, (i % 2) * perTextureHeight, perTextureWidth, perTextureHeight });
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
