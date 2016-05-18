// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

//////////////////////////////////////////////////////////////////////////////////////
// librealsense Multi-threading Demo 2 - duplicating frames without extra copies    //
//////////////////////////////////////////////////////////////////////////////////////

#include <librealsense/rs.hpp>
#include <cstdio>
#include <GLFW/glfw3.h>
#include "concurrency.hpp"
#include <thread>
#include <iostream>

int main() try
{
    // In this demo each frame is being rendered by several different consumers
    // Please note that only change from the previous demo is that now we are submitting N _clones_ of the original frameset

    rs::context ctx;
    printf("There are %d connected RealSense devices.\n", ctx.get_device_count());
    if(ctx.get_device_count() == 0) return EXIT_FAILURE;

    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
    printf("    Serial number: %s\n", dev->get_serial());
    printf("    Firmware version: %s\n", dev->get_firmware_version());

    // create N queues for the N consumers
    const auto consumers = 2;
    single_consumer_queue<rs::frameset> frames_queue[consumers];

    auto running = true;
    glfwInit();

    for (auto i = 0; i < consumers; i++)
    {
        // Each consumer will dequeue a copy of every frame produced by the device
        std::thread consumer([dev, &frames_queue, &running, i]()
        {
            try
            {
                GLFWwindow * win = glfwCreateWindow(1280, 960, "librealsense - multi-threading demo-2", nullptr, nullptr);
                glfwMakeContextCurrent(win);
                while (!glfwWindowShouldClose(win))
                {
                    glfwPollEvents();
                    auto frames = frames_queue[i].dequeue();

                    glClear(GL_COLOR_BUFFER_BIT);
                    glPixelZoom(1, -1);

                    glRasterPos2f(-1, 1);
                    glPixelTransferf(GL_RED_SCALE, 0xFFFF * dev->get_depth_scale() / 2.0f);
                    glDrawPixels(640, 480, GL_RED, GL_UNSIGNED_SHORT, frames.get_frame_data(rs::stream::depth));
                    glPixelTransferf(GL_RED_SCALE, 1.0f);

                    glRasterPos2f(0, 1);
                    glDrawPixels(640, 480, GL_RGB, GL_UNSIGNED_BYTE, frames.get_frame_data(rs::stream::color));

                    glRasterPos2f(-1, 0);
                    glDrawPixels(640, 480, GL_LUMINANCE, GL_UNSIGNED_BYTE, frames.get_frame_data(rs::stream::infrared));

                    if (dev->is_stream_enabled(rs::stream::infrared2))
                    {
                        glRasterPos2f(0, 0);
                        glDrawPixels(640, 480, GL_LUMINANCE, GL_UNSIGNED_BYTE, frames.get_frame_data(rs::stream::infrared2));
                    }

                    glfwSwapBuffers(win);
                }
            }
            catch (const rs::error & e)
            {
                printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
                printf("    %s\n", e.what());
            }
            running = false;
        });
        consumer.detach();
    }

    dev->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 30);
    dev->enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 30);
    dev->enable_stream(rs::stream::infrared, 640, 480, rs::format::y8, 30);
    try { dev->enable_stream(rs::stream::infrared2, 640, 480, rs::format::y8, 60); }
    catch(...) { printf("Device does not provide infrared2 stream.\n"); }
    dev->start();

    while (running)
    {
        auto frames = dev->wait_for_frames_safe();

        for (auto i = 0; i < consumers; i++)
        {
            // since we are increasing the workload and hence frame lifetime sagnificatly, 
            // clone operation might fail when there are no available frame references left (for example if the thread is stuck and the queue keeps filling up)
            /*rs::frameset clone;
            if (frames.try_clone(clone)) 
            {
                frames_queue[i].enqueue(std::move(clone));
            }*/

            // alternatively we can limit queue size:
            if (frames_queue[i].size() <= 4)
            {
                frames_queue[i].enqueue(std::move(frames.clone()));
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
