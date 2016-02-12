// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include "example.hpp"

#include <iostream>
#include <algorithm>

std::vector<texture_buffer> buffers;
std::vector<rs::device *> devices;

int main(int argc, char * argv[]) try
{
    rs::log_to_console(rs::log_severity::debug);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    if(ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    
    // Enumerate all devices
    for(int i=0; i<ctx.get_device_count(); ++i)
    {
        devices.push_back(ctx.get_device(i));
    }

    // Configure and start our devices
    for(auto dev : devices)
    {
        std::cout << "Starting " << dev->get_name() << "... ";
        dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
        dev->enable_stream(rs::stream::color, rs::preset::best_quality);
        dev->start();
        std::cout << "done." << std::endl;
    }

	// Depth and color
    buffers.resize(ctx.get_device_count() * 2);

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Multi-Camera Example";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwSetWindowUserPointer(win, &ctx);
    glfwSetKeyCallback(win, [](GLFWwindow * win, int key, int scancode, int action, int mods) 
    { 
        auto ctx = (rs::context *)glfwGetWindowUserPointer(win);
        if(action != GLFW_RELEASE)
        {
            if(key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
            {
                int index = key - GLFW_KEY_0;
                if(index < ctx->get_device_count())
                {
                    auto * dev = ctx->get_device(index);
                    std::cout << "Resetting device " << index << " (" << dev->get_name() << ")" << std::endl;
                    dev->reset();

                    std::cout << "Device reset. Restarting streaming..." << std::endl;
                    dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
                    dev->enable_stream(rs::stream::color, rs::preset::best_quality);
                    dev->start();
                }
            }

            if(key == GLFW_KEY_R)
            {
                int old_count = ctx->get_device_count();
                ctx->enumerate_devices();
                int new_count = ctx->get_device_count();
                for(int i=old_count; i<new_count; ++i)
                {
                    auto dev = ctx->get_device(i);

                    devices.push_back(dev);
                    std::cout << "Starting " << dev->get_name() << "... ";
                    dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
                    dev->enable_stream(rs::stream::color, rs::preset::best_quality);
                    dev->start();
                    std::cout << "done." << std::endl;
                }
                buffers.resize(new_count * 2);
            }
        }
    });
    glfwMakeContextCurrent(win);

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

	    // Does not account for correct aspect ratios
	    auto perTextureWidth = w / devices.size();
	    auto perTextureHeight = h / 2;

        int i=0, x=0;
        for(auto dev : devices)
        {
            dev->poll_for_frames();
            const auto c = dev->get_stream_intrinsics(rs::stream::color), d = dev->get_stream_intrinsics(rs::stream::depth);
            buffers[i++].show(*dev, rs::stream::color, x, 0, perTextureWidth, perTextureHeight);
            buffers[i++].show(*dev, rs::stream::depth, x, perTextureHeight, perTextureWidth, perTextureHeight);
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