// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include "example.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <map>
#include <memory>

texture_buffer buffers[RS_STREAM_COUNT];
bool align_depth_to_color = false;
bool align_color_to_depth = false;
bool color_rectification_enabled = false;

// Split the screen into 640X480 tiles, according to the number of supported streams. Define layout as follows : tiles -> <columds,rows>
const std::map<size_t, std::pair<int, int>> tiles_map = {       { 1,{ 1,1 } },
                                                                { 2,{ 2,1 } },
                                                                { 3,{ 2,2 } },
                                                                { 4,{ 2,2 } },
                                                                { 5,{ 3,2 } },          // E.g. five tiles, split into 3 columns by 2 rows mosaic
                                                                { 6,{ 3,2 } }};

int main(int argc, char * argv[]) try
{
    rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    if (ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    rs::device & dev = *ctx.get_device(0);

    std::vector<rs::stream> supported_streams;

    for (int i = (int)rs::capabilities::depth; i <= (int)rs::capabilities::fish_eye; i++)
        if (dev.supports((rs::capabilities)i))
            supported_streams.push_back((rs::stream)i);

    // Configure all supported streams to run at 30 frames per second
    for (auto & stream : supported_streams)
        dev.enable_stream(stream, rs::preset::best_quality);

    // Compute field of view for each enabled stream
    for (auto & stream : supported_streams)
    {
        if (!dev.is_stream_enabled(stream)) continue;
        auto intrin = dev.get_stream_intrinsics(stream);
        std::cout << "Capturing " << stream << " at " << intrin.width << " x " << intrin.height;
        std::cout << std::setprecision(1) << std::fixed << ", fov = " << intrin.hfov() << " x " << intrin.vfov() << ", distortion = " << intrin.model() << std::endl;
    }

    // Start our device
    dev.start();

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Capture Example (" << dev.get_name() << ")";

    int rows = tiles_map.at(supported_streams.size()).second;
    int cols = tiles_map.at(supported_streams.size()).first;
    int tile_w = 640; // pixels
    int tile_h = 480; // pixels
    GLFWwindow * win = glfwCreateWindow(tile_w*cols, tile_h*rows, ss.str().c_str(), 0, 0);
    glfwSetWindowUserPointer(win, &dev);
    glfwSetKeyCallback(win, [](GLFWwindow * win, int key, int scancode, int action, int mods)
    {
        auto dev = reinterpret_cast<rs::device *>(glfwGetWindowUserPointer(win));
        if (action != GLFW_RELEASE) switch (key)
        {
        case GLFW_KEY_R: color_rectification_enabled = !color_rectification_enabled; break;
        case GLFW_KEY_C: align_color_to_depth = !align_color_to_depth; break;
        case GLFW_KEY_D: align_depth_to_color = !align_depth_to_color; break;
        case GLFW_KEY_E:
            if (dev->supports_option(rs::option::r200_emitter_enabled))
            {
                int value = !dev->get_option(rs::option::r200_emitter_enabled);
                std::cout << "Setting emitter to " << value << std::endl;
                dev->set_option(rs::option::r200_emitter_enabled, value);
            }
            break;
        case GLFW_KEY_A:
            if (dev->supports_option(rs::option::r200_lr_auto_exposure_enabled))
            {
                int value = !dev->get_option(rs::option::r200_lr_auto_exposure_enabled);
                std::cout << "Setting auto exposure to " << value << std::endl;
                dev->set_option(rs::option::r200_lr_auto_exposure_enabled, value);
            }
            break;
        }
    });
    glfwMakeContextCurrent(win);

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();
        dev.wait_for_frames();

        // Clear the framebuffer
        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the images
        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);
        buffers[0].show(dev, align_color_to_depth ? rs::stream::color_aligned_to_depth : (color_rectification_enabled ? rs::stream::rectified_color : rs::stream::color), 0, 0, tile_w, tile_h);
        buffers[1].show(dev, align_depth_to_color ? (color_rectification_enabled ? rs::stream::depth_aligned_to_rectified_color : rs::stream::depth_aligned_to_color) : rs::stream::depth, w / cols, 0, tile_w, tile_h);
        buffers[2].show(dev, rs::stream::infrared, 0, h / rows, tile_w, tile_h);
        buffers[3].show(dev, rs::stream::infrared2, w / cols, h / rows, tile_w, tile_h);
        if (dev.is_stream_enabled(rs::stream::fisheye))
            buffers[4].show(dev, rs::stream::fisheye, 2 * w / cols, 0, tile_w, tile_h);
        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch (const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
