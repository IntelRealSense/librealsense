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


int main(int argc, char * argv[])
{
    context ctx;
    
    util::device_hub hub(ctx);

    auto finished = false;
    GLFWwindow* win;
    while (!finished)
    {
        try
        {
            auto dev = hub.wait_for_device();

            // Configure all supported streams to run at 30 frames per second
            util::config config;
            config.enable_all(preset::best_quality);

            auto stream = config.open(dev);

            auto black = ctx.create_processing_block(RS2_EXTENSION_TYPE_VIDEO_FRAME,
                [](std::vector<frame> frames, const frame_source& source)
            {
                auto&& first = frames.front();
                auto result = source.allocate_video_frame(first.get_stream_type(), first);
                auto vf = first.as<video_frame>();
                auto rf = result.as<video_frame>();
                auto data = (uint8_t*)rf.get_data();
                memset(data, 0, rf.get_stride_in_bytes() * rf.get_height());
                auto orig_data = (uint8_t*)vf.get_data();
                auto pixels = vf.get_width() * vf.get_height();
                for (auto i = 0; i < 0.2 * pixels; i++)
                {
                    auto x = rand() % vf.get_width();
                    auto y = rand() % vf.get_height();
                    for (auto c = 0; c < vf.get_bytes_per_pixel(); c++)
                    {
                        auto orig_pixel = orig_data[y * vf.get_stride_in_bytes() + x * vf.get_bytes_per_pixel() + c];
                        data[y * vf.get_stride_in_bytes() + x * vf.get_bytes_per_pixel() + c] = orig_pixel;
                    }
                }
                source.frame_ready(std::move(result));
            });

			syncer_processing_block syncer;
            stream.start(syncer);

           // black.start(syncer);
			frame_queue queue;
			syncer.start(queue);
            texture_buffer buffers[RS2_STREAM_COUNT];

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

                auto index = 0;
				auto frame = queue.wait_for_frame();
                //// for consistent visualization, sort frames based on stream type:
                //sort(frames.begin(), frames.end(),
                //     [](const frame& a, const frame& b) -> bool
                //{
                //    return a.get_stream_type() < b.get_stream_type();
                //});

                ////dev.get_option(RS2_OPTION_LASER_POWER);
                auto tiles_horisontal = static_cast<int>(ceil(sqrt(1)));
                auto tiles_vertical = ceil((float)1 / tiles_horisontal);
                auto tile_w = static_cast<float>((float)w / tiles_horisontal);
                auto tile_h = static_cast<float>((float)h / tiles_vertical);

              /*  for (auto&& frame : frames)
                {*/
                    auto stream_type = frame.get_stream_type();
                    buffers[stream_type].upload(frame);
               /* }*/

                // Wait for new images
                glfwPollEvents();

                // Clear the framebuffer
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                // Draw the images
                glPushMatrix();
                glfwGetWindowSize(win, &w, &h);
                glOrtho(0, w, h, 0, -1, +1);

                index = 0;
               /* for (auto&& frame : frames)
                {*/
                    stream_type = frame.get_stream_type();
                    auto col_id = index / tiles_horisontal;
                    auto row_id = index % tiles_horisontal;

                    buffers[stream_type].show({ row_id * tile_w, static_cast<float>(col_id * tile_h), tile_w, tile_h }, 1);

                    index++;
               /* }*/

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

