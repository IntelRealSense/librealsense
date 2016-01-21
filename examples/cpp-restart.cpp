// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include "example.hpp"

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

texture_buffer buffers[RS_STREAM_COUNT];

int main(int argc, char * argv[]) try
{
    rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    if(ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    rs::device & dev = *ctx.get_device(0);

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Restart Example (" << dev.get_name() << ")";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    for(int i=0; i<20; ++i)
    {
        try
        {
            if(dev.is_streaming()) dev.stop();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            for(int j=0; j<4; ++j)
            {
                auto s = (rs::stream)j;
                if(dev.is_stream_enabled(s)) dev.disable_stream(s);
            }

            switch(i)
            {
            case 0:
                dev.enable_stream(rs::stream::color, 640, 480, rs::format::yuyv, 60);
                break;
            case 1:
                dev.enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 60);
                break;
            case 2:
                dev.enable_stream(rs::stream::color, 640, 480, rs::format::bgr8, 60);
                break;
            case 3:
                dev.enable_stream(rs::stream::color, 640, 480, rs::format::rgba8, 60);
                break;
            case 4:
                dev.enable_stream(rs::stream::color, 640, 480, rs::format::bgra8, 60);
                break;
            case 5:
                dev.enable_stream(rs::stream::color, 1920, 1080, rs::format::rgb8, 0);
                break;
            case 6:
                dev.enable_stream(rs::stream::depth, rs::preset::largest_image);
                break;
            case 7:
                dev.enable_stream(rs::stream::depth, 480, 360, rs::format::z16, 60);
                break;
            case 8:
                dev.enable_stream(rs::stream::depth, 320, 240, rs::format::z16, 60);
                break;
            case 9:
                dev.enable_stream(rs::stream::infrared, rs::preset::largest_image);
                break;
            case 10:
                dev.enable_stream(rs::stream::infrared, 492, 372, rs::format::y8, 60);
                break;
            case 11:
                dev.enable_stream(rs::stream::infrared, 320, 240, rs::format::y8, 60);
                break;
            case 12:
                dev.enable_stream(rs::stream::infrared, 0, 0, rs::format::y16, 60);
                break;
            case 13:
                dev.enable_stream(rs::stream::infrared, 0, 0, rs::format::y8, 60);
                dev.enable_stream(rs::stream::infrared2, 0, 0, rs::format::y8, 60);
                break;
            case 14:
                dev.enable_stream(rs::stream::infrared, 0, 0, rs::format::y16, 60);
                dev.enable_stream(rs::stream::infrared2, 0, 0, rs::format::y16, 60);
                break;
            case 15:
                dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
                dev.enable_stream(rs::stream::infrared, 0, 0, rs::format::y8, 0);
                break;
            case 16:
                dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
                dev.enable_stream(rs::stream::infrared, 0, 0, rs::format::y16, 0);
                break;
            case 17:
                dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
                dev.enable_stream(rs::stream::color, rs::preset::best_quality);
                break;
            case 18:
                dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
                dev.enable_stream(rs::stream::color, rs::preset::best_quality);
                dev.enable_stream(rs::stream::infrared, rs::preset::best_quality);
                break;
            case 19:
                dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
                dev.enable_stream(rs::stream::color, rs::preset::best_quality);
                dev.enable_stream(rs::stream::infrared, rs::preset::best_quality);
                dev.enable_stream(rs::stream::infrared2, rs::preset::best_quality);
                break;
            }

            dev.start();
            for(int j=0; j<120; ++j)
            {
                // Wait for new images
                glfwPollEvents();
                if(glfwWindowShouldClose(win)) goto done;
                dev.wait_for_frames();

                // Clear the framebuffer
                int w,h;
                glfwGetWindowSize(win, &w, &h);
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                // Draw the images
                glPushMatrix();
                glfwGetWindowSize(win, &w, &h);
                glOrtho(0, w, h, 0, -1, +1);
                buffers[0].show(dev, rs::stream::color, 0, 0, w/2, h/2);
                buffers[1].show(dev, rs::stream::depth, w/2, 0, w-w/2, h/2);
                buffers[2].show(dev, rs::stream::infrared, 0, h/2, w/2, h-h/2);
                buffers[3].show(dev, rs::stream::infrared2, w/2, h/2, w-w/2, h-h/2);
                glPopMatrix();
                glfwSwapBuffers(win);
            }
        }
        catch(const rs::error & e)
        {
            std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
            std::cout << "Skipping mode " << i << std::endl;
        }
    }
done:
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
