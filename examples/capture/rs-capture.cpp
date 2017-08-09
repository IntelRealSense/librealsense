// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"

#include <sstream>
#include <iostream>
#include <memory>

using namespace rs2;
using namespace std;

//std::vector<rs2::stream_profile> configure_all_supported_streams(rs2::device& dev, util::config& config , int width = 640,  int height = 480, int fps = 60)
//{
//    std::vector<rs2::stream_profile> profiles;
//
//    if (config.can_enable_stream(dev, RS2_STREAM_DEPTH, width, height, 15, RS2_FORMAT_Z16))
//    {
//        config.enable_stream( RS2_STREAM_DEPTH, width, height, 15, RS2_FORMAT_Z16);
//        profiles.push_back({RS2_STREAM_DEPTH, width, height, 15, RS2_FORMAT_Z16});
//    }
//    if (config.can_enable_stream(dev, RS2_STREAM_COLOR, width, height, 30, RS2_FORMAT_RGB8))
//    {
//        config.enable_stream( RS2_STREAM_COLOR, width, height, 30, RS2_FORMAT_RGB8);
//        profiles.push_back({RS2_STREAM_COLOR, width, height, 30, RS2_FORMAT_RGB8});
//    }
//    if (config.can_enable_stream(dev, RS2_STREAM_INFRARED, width, height, 15, RS2_FORMAT_Y8))
//    {
//        config.enable_stream(  RS2_STREAM_INFRARED, width, height, 15, RS2_FORMAT_Y8);
//        profiles.push_back({ RS2_STREAM_INFRARED, width, height, 15, RS2_FORMAT_Y8});
//    }
//    if (config.can_enable_stream(dev, RS2_STREAM_INFRARED2, width, height, 15, RS2_FORMAT_Y8))
//    {
//        config.enable_stream(  RS2_STREAM_INFRARED2, width, height, 15, RS2_FORMAT_Y8);
//        profiles.push_back({ RS2_STREAM_INFRARED2, width, height, 15, RS2_FORMAT_Y8});
//    }
//    if (config.can_enable_stream(dev, RS2_STREAM_FISHEYE, width, height, fps, RS2_FORMAT_RAW8))
//    {
//        config.enable_stream( RS2_STREAM_FISHEYE, width, height, fps, RS2_FORMAT_RAW8);
//        profiles.push_back({RS2_STREAM_FISHEYE, width, height, fps, RS2_FORMAT_RAW8});
//    }
//    if (config.can_enable_stream(dev, RS2_STREAM_GYRO, 0, 0, 0, RS2_FORMAT_MOTION_XYZ32F))
//    {
//        config.enable_stream( RS2_STREAM_GYRO,  1, 1, 1000, RS2_FORMAT_MOTION_XYZ32F);
//        profiles.push_back({RS2_STREAM_GYRO,  0, 0, 0, RS2_FORMAT_MOTION_XYZ32F});
//    }
//    if (config.can_enable_stream(dev, RS2_STREAM_ACCEL,  0, 0, 0, RS2_FORMAT_MOTION_XYZ32F))
//    {
//        config.enable_stream( RS2_STREAM_ACCEL, 1, 1, 1000,  RS2_FORMAT_MOTION_XYZ32F);
//        profiles.push_back({RS2_STREAM_ACCEL,  0, 0, 0, RS2_FORMAT_MOTION_XYZ32F});
//    }
//    return profiles;
//}

int main(int argc, char * argv[])
{
    context ctx;
    util::device_hub hub(ctx);

    auto finished = false;
    GLFWwindow* win;
    log_to_file(RS2_LOG_SEVERITY_DEBUG);

    while (!finished)
    {
        try
        {
            auto dev = hub.wait_for_device();

            // Configure all supported streams to run at 30 frames per second
            util::config config;
            config.enable_all(preset::best_quality);
            //configure_all_supported_streams(dev, config);
            auto stream = config.open(dev);

            syncer_processing_block syncer;
            stream.start(syncer);

            size_t max_frames = 0;

           // black.start(syncer);
            frame_queue queue;
            syncer.start(queue);
            map<int, texture_buffer> buffers;
            bool is_stream_active[RS2_STREAM_COUNT] = {false};
            // Open a GLFW window
            glfwInit();
            ostringstream ss;
            ss << "CPP Capture Example (" << dev.get_info(RS2_CAMERA_INFO_NAME) << ")";

            win = glfwCreateWindow(1280, 720, ss.str().c_str(), nullptr, nullptr);
            glfwMakeContextCurrent(win);

            while (hub.is_connected(dev) && !glfwWindowShouldClose(win))
            {
                int w, h;
                glfwGetFramebufferSize(win, &w, &h);

                auto frames = queue.wait_for_frames();

                max_frames = std::max(max_frames, frames.size());

                //// for consistent visualization, sort frames based on stream type:
                sort(frames.begin(), frames.end(),
                     [](const frame& a, const frame& b) -> bool
                {
                    return a.get_profile().unique_id() < b.get_profile().unique_id();
                });


                for (auto&& frame : frames)
                {
                    buffers[frame.get_profile().unique_id()].upload(frame);
                    is_stream_active[frame.get_profile().stream_type()] = true;
                }

                auto tiles_horisontal = static_cast<int>(ceil(sqrt(buffers.size())));
                auto tiles_vertical = ceil(static_cast<float>(buffers.size()) / tiles_horisontal);
                auto tile_w = static_cast<float>(static_cast<float>(w) / tiles_horisontal);
                auto tile_h = static_cast<float>(static_cast<float>(h) / tiles_vertical);

                // Wait for new images
                glfwPollEvents();

                // Clear the framebuffer
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                // Draw the images
                glPushMatrix();
                glfwGetWindowSize(win, &w, &h);
                glOrtho(0, w, h, 0, -1, +1);

                for (auto i = 0; i< RS2_STREAM_COUNT; i++)
                {
                    auto stream_index = i;

                    auto index = distance(begin(buffers), buffers.find(stream_index));
                    auto col_id = index / tiles_horisontal;
                    auto row_id = index % tiles_horisontal;

                    buffers[stream_index].show({ row_id * tile_w, static_cast<float>(col_id * tile_h), tile_w, tile_h }, 1);
                        buffers[i].show({ row_id * tile_w, static_cast<float>(col_id * tile_h), tile_w, tile_h }, 1);
                }
                glPopMatrix();
                glfwSwapBuffers(win);
            }

            if (glfwWindowShouldClose(win))
                finished = true;
        }
        catch (const error & e)
        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
        }
        catch (const exception & e)
        {
            cerr << e.what() << endl;
        }
        glfwDestroyWindow(win);
        glfwTerminate();
    }

    return EXIT_SUCCESS;
}
