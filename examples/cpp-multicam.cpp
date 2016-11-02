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

    std::vector<rs::util::multistream> streams;
    std::vector<rs::util::syncer> syncers;

    // Configure and start our devices
    for(auto&& dev : devices)
    {
        std::cout << "Starting " << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << "... ";
        rs::util::config config;
        config.enable_stream(RS_STREAM_DEPTH, rs::preset::best_quality);
        config.enable_stream(RS_STREAM_COLOR, rs::preset::best_quality);
        streams.push_back(config.open(dev));
        syncers.emplace_back();
        streams.back().start(syncers.back());
        std::cout << "done." << std::endl;
    }

    // Depth and color
    buffers.resize(devices.size() * 2);

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Multi-Camera Example";
    auto win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    int windowWidth, windowHeight;
    glfwGetWindowSize(win, &windowWidth, &windowHeight);

    // Does not account for correct aspect ratios
    auto perTextureWidth = static_cast<float>(windowWidth / devices.size());
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
        auto x = 0.0f;

        for(auto&& syncer : syncers)
        {
            rs::util::frameset frames;
            if (syncer.poll_for_frames(frames))
            {
                auto j = i;
                for (auto&& frame : frames)
                {
                    buffers[j++].upload(frame);
                }
            }
            //const auto c = dev->get_stream_intrinsics(rs::stream::color), d = dev->get_stream_intrinsics(rs::stream::depth);
            buffers[i++].show({ x, 0, perTextureWidth, perTextureHeight });
            buffers[i++].show({ x, perTextureHeight, perTextureWidth, perTextureHeight });
            x += perTextureWidth;
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
