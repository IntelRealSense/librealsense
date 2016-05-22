// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

//////////////////////////////////////////////////////////////////////////////////////
// librealsense Multi-threading Demo 3 - per-frame pipeline                         //
//////////////////////////////////////////////////////////////////////////////////////

#include <librealsense/rs.hpp>
#include <cstdio>
#include <GLFW/glfw3.h>
#include "concurrency.hpp"
#include <thread>
#include <iostream>

int main() try
{
    // In this demo we have 4 consumers, rendering RGB, Depth, IR1 and IR2 independently of each other 

    rs::context ctx;
    printf("There are %d connected RealSense devices.\n", ctx.get_device_count());
    if(ctx.get_device_count() == 0) return EXIT_FAILURE;

    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
    printf("    Serial number: %s\n", dev->get_serial());
    printf("    Firmware version: %s\n", dev->get_firmware_version());

    // create N queues for the N consumers
    const auto consumers = 3;
    single_consumer_queue<rs::frame> frames_queue[consumers];
    std::vector<bool> running(consumers, true);

    glfwInit();

    for (auto i = 0; i < consumers; i++)
    {
        std::thread consumer([dev, &frames_queue, &running, i]()
        {
            try
            {
                GLFWwindow * win = glfwCreateWindow(640, 480, "librealsense - multi-threading demo-3", nullptr, nullptr);
                glfwMakeContextCurrent(win);
                while (!glfwWindowShouldClose(win))
                {
                    glfwPollEvents();
                    auto frame = frames_queue[i].dequeue();

                    glClear(GL_COLOR_BUFFER_BIT);
                    glPixelZoom(1, -1);

                    glRasterPos2f(-1, 1);

                    if ((rs::stream)i == rs::stream::depth)
                    {
                        glPixelTransferf(GL_RED_SCALE, 0xFFFF * dev->get_depth_scale() / 2.0f);
                        glDrawPixels(dev->get_stream_width(rs::stream::depth), dev->get_stream_height(rs::stream::depth), GL_RED, GL_UNSIGNED_SHORT, frame.get_frame_data());
                        glPixelTransferf(GL_RED_SCALE, 1.0f);
                    }

                    if ((rs::stream)i == rs::stream::color)
                    {
                        glDrawPixels(dev->get_stream_width(rs::stream::color), dev->get_stream_height(rs::stream::color), GL_RGB, GL_UNSIGNED_BYTE, frame.get_frame_data());
                    }

                    if ((rs::stream)i == rs::stream::infrared)
                    {
                        glDrawPixels(dev->get_stream_width(rs::stream::infrared), dev->get_stream_height(rs::stream::infrared), GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.get_frame_data());
                    }

                    glfwSwapBuffers(win);
                }
            }
            catch (const rs::error & e)
            {
                printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
                printf("    %s\n", e.what());
            }
            running[i] = false;
        });
        consumer.detach();
    }

    dev->enable_stream(rs::stream::depth, 0, 0, rs::format::z16, 30);
    dev->enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 30);
    dev->enable_stream(rs::stream::infrared, 0, 0, rs::format::y8, 30);
    dev->start();

    while (any_costumers_alive(running))
    {
        auto frames = dev->wait_for_frames_safe();
        for (auto i = 0; i < consumers; i++)
        {
            if (frames_queue[i].size() <= 4)
            {
                frames_queue[i].enqueue(std::move(frames.detach_frame((rs::stream)i)));
            }
        }
    }

    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
    printf("    %s\n", e.what());
    return EXIT_FAILURE;
}
