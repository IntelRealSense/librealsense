// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"
#include "model-views.h"

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

            // Configure depth stream to run at 30 frames per second
            util::config config;
            config.enable_stream(RS2_STREAM_DEPTH, 640, 480, 30, RS2_FORMAT_Z16);

            auto stream = config.open(dev);

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

            /************************
             ***  GUI CODE - BEGIN
             ************************/

            ImGui_ImplGlfw_Init(win, true);


            /************************
             ***  GUI CODE - END
             ************************/



            while (hub.is_connected(dev) && !glfwWindowShouldClose(win))
            {
                
                int w, h;
                glfwGetFramebufferSize(win, &w, &h);

                auto index = 0;
				auto frames = queue.wait_for_frames();

                for (auto&& frame : frames)
                {
                    auto stream_type = frame.get_stream_type();
                    buffers[stream_type].upload(frame);
                }

                // Wait for new images
                glfwPollEvents();

                // Clear the framebuffer
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                // Draw the images
                glPushMatrix();
                glfwGetWindowSize(win, &w, &h);
                glOrtho(0, w, h, 0, -1, +1);

                for (auto&& frame : frames)
                {
                    auto stream_type = frame.get_stream_type();
                    buffers[stream_type].show({ 0, 0, w, h }, 1);
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

