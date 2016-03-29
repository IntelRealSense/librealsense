// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include "example.hpp"
#include <librealsense/rs.h>
#include <librealsense/rsutil.h>

#include <iostream>
#include <algorithm>

std::vector<texture_buffer> buffers;

int main(int argc, char * argv[]) try
{
    rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    if(ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    
    // Enumerate all devices
    std::vector<rs::device *> devices;
    for(int i=0; i<ctx.get_device_count(); ++i)
    {
        devices.push_back(ctx.get_device(i));
    }

    // Configure and start our devices
    int perTextureWidth = 0;
    int perTextureHeight = 0;

    for(auto dev : devices)
    {
        std::cout << "Starting " << dev->get_name() << "... ";
        dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
        dev->start();
        std::cout << "done." << std::endl;
        perTextureWidth = dev->get_stream_width(rs::stream::depth);
        perTextureHeight = dev->get_stream_height(rs::stream::depth);
    }
    // Depth and color
    buffers.resize(ctx.get_device_count());

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Multi-Camera Example";
    GLFWwindow * win = glfwCreateWindow(perTextureWidth, perTextureHeight, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    int windowWidth, windowHeight;
    glfwGetWindowSize(win, &windowWidth, &windowHeight);

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
        for(auto dev : devices)
        {
            dev->poll_for_frames();
         
            buffers[0].show(*dev, rs::stream::depth, 0, 0, (int)perTextureWidth, (int)perTextureHeight);
           
            const float scale = rs_get_device_depth_scale((const rs_device*)dev, NULL);
            const uint16_t * image = (const uint16_t *)rs_get_frame_data((const rs_device*)dev, RS_STREAM_DEPTH, NULL);

            for (int x =  perTextureHeight / 2;
                    x < perTextureWidth + perTextureHeight / 2; x++){
                float depth_in_meters = scale * image[x];
             
                if(depth_in_meters > 0){
                    std::printf("%.6f \n ", depth_in_meters);
                    std::printf("%d \n \n \n", x);
                
                    glBegin(GL_POINTS);
                    glVertex3f(x % perTextureWidth, perTextureHeight / 2, 0);
                    glEnd( );
                    
                }
            }
            int draw_y = 0;
            for (int _y = perTextureWidth / 2; _y < ((perTextureHeight + perTextureHeight) * perTextureWidth / 2) ; _y += perTextureWidth){
                float depth_in_meters = scale * image[_y];
                if(depth_in_meters > 0){
                    std::printf("%.6f \n ", depth_in_meters);
                    std::printf("%d \n \n \n", _y);
                
                    glBegin(GL_POINTS);
                    glVertex3f(perTextureWidth / 2, draw_y, 0);
                    glEnd( );
                }
                draw_y++;
            }
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
