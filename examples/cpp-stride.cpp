// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

//////////////////////////////////////////////////////////////////////////////////////
// librealsense Multi-threading Demo 4 - low latency with callbacks and zero copy   //
//////////////////////////////////////////////////////////////////////////////////////

#include <librealsense/rs.hpp>
#include <cstdio>
#include <atomic>
#include <map>
#include <GLFW/glfw3.h>

#include "concurrency.hpp"
#include "example.hpp"
#include <iostream>

int main() try
{
    rs::context ctx;
    printf("There are %d connected RealSense devices.\n", ctx.get_device_count());
    if (ctx.get_device_count() == 0) return EXIT_FAILURE;

    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
    printf("    Serial number: %s\n", dev->get_serial());
    printf("    Firmware version: %s\n", dev->get_firmware_version());

    const auto streams = 4;
    std::vector<uint16_t> supported_streams = { (uint16_t)rs::stream::depth, (uint16_t)rs::stream::color, (uint16_t)rs::stream::infrared};
    const size_t max_queue_size = 1; // To minimize latency prefer frame drops
    single_consumer_queue<rs::frame> frames_queue[streams];
    texture_buffer buffers[streams];
    std::atomic<bool> running(true);

    struct resolution
    {
        int width;
        int height;
        rs::format format;
    };
    std::map<rs::stream, resolution> resolutions;

    if(dev->supports(rs::capabilities::infrared2))
            supported_streams.push_back((uint16_t)rs::stream::infrared2);

    for (auto i : supported_streams)
    {
        dev->set_frame_callback((rs::stream)i, [dev, &running, &frames_queue, &resolutions, i, max_queue_size](rs::frame frame)
        {
            if (running && frames_queue[i].size() <= max_queue_size) frames_queue[i].enqueue(std::move(frame));
        });
    }

    dev->enable_stream(rs::stream::depth, 0, 0, rs::format::z16, 60, rs::output_buffer_format::native);
    dev->enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 60, rs::output_buffer_format::native);
    dev->enable_stream(rs::stream::infrared, 0, 0, rs::format::y8, 60, rs::output_buffer_format::native);
    if (dev->supports(rs::capabilities::infrared2))
        dev->enable_stream(rs::stream::infrared2, 0, 0, rs::format::y8, 60, rs::output_buffer_format::native);

    resolutions[rs::stream::depth] = { dev->get_stream_width(rs::stream::depth), dev->get_stream_height(rs::stream::depth), rs::format::z16 };
    resolutions[rs::stream::color] = { dev->get_stream_width(rs::stream::color), dev->get_stream_height(rs::stream::color), rs::format::rgb8 };
    resolutions[rs::stream::infrared] = { dev->get_stream_width(rs::stream::infrared), dev->get_stream_height(rs::stream::infrared), rs::format::y8 };
    if(dev->supports(rs::capabilities::infrared2))
        resolutions[rs::stream::infrared2] = { dev->get_stream_width(rs::stream::infrared2), dev->get_stream_height(rs::stream::infrared2), rs::format::y8 };

    glfwInit();

    auto max_aspect_ratio = 0.0f;
    for (auto i : supported_streams)
    {
        auto aspect_ratio = static_cast<float>(resolutions[static_cast<rs::stream>(i)].height) / static_cast<float>(resolutions[static_cast<rs::stream>(i)].width);
        if (max_aspect_ratio < aspect_ratio)
            max_aspect_ratio = aspect_ratio;
    };

    auto win = glfwCreateWindow(1100, int(1100 * max_aspect_ratio), "librealsense - stride", nullptr, nullptr);
    glfwMakeContextCurrent(win);

    dev->start();

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        rs::frame frame;

        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(win, &w, &h);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, +1);

        for (auto i = 0; i < streams; i++)
        {
            if(!dev->supports(rs::capabilities(i))) continue;

            auto res = resolutions[(rs::stream)i];

            if (frames_queue[i].try_dequeue(&frame))
            {
                buffers[i].upload(frame);
            }

            auto x = (i % 2) * (w / 2);
            auto y = (i / 2) * (h / 2);
            buffers[i].show(x, y, w / 2, h / 2, res.width, res.height);
        }

        glfwSwapBuffers(win);
    }

    running = false;

    for (auto i : supported_streams) frames_queue[i].clear();

    dev->stop();

    for (auto i : supported_streams)
    {
        if (dev->is_stream_enabled((rs::stream)i))
            dev->disable_stream((rs::stream)i);
    }

    return EXIT_SUCCESS;
}
catch (const rs::error & e)
{
    printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
    printf("    %s\n", e.what());
    return EXIT_FAILURE;
}
