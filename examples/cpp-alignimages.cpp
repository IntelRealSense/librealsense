// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include "example.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

texture_buffer buffers[6];

#pragma pack(push, 1)
struct rgb_pixel
{
    uint8_t r,g,b; 
};
#pragma pack(pop)

int main(int argc, char * argv[]) try
{
    rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    if(ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    rs::device & dev = *ctx.get_device(0);

    dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
    dev.enable_stream(rs::stream::color, rs::preset::best_quality);
    try { dev.enable_stream(rs::stream::infrared2, rs::preset::best_quality); } catch(...) {}
    dev.start();

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Image Alignment Example (" << dev.get_name() << ")";
    GLFWwindow * win = glfwCreateWindow(dev.is_stream_enabled(rs::stream::infrared2) ? 1920 : 1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();
        dev.wait_for_frames();

        // Clear the framebuffer
        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the images        
        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);
        int s = w / (dev.is_stream_enabled(rs::stream::infrared2) ? 3 : 2);
        buffers[0].show(dev, rs::stream::color, 0, 0, s, h-h/2);
        buffers[1].show(dev, rs::stream::color_aligned_to_depth, s, 0, s, h-h/2);
        buffers[2].show(dev, rs::stream::depth_aligned_to_color, 0, h/2, s, h-h/2);
        buffers[3].show(dev, rs::stream::depth, s, h/2, s, h-h/2);
        if(dev.is_stream_enabled(rs::stream::infrared2))
        {
            buffers[4].show(dev, rs::stream::infrared2_aligned_to_depth, 2*s, 0, s, h-h/2);
            buffers[5].show(dev, rs::stream::depth_aligned_to_infrared2, 2*s, h/2, s, h-h/2);
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