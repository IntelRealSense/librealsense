#include <librealsense/rs.hpp>
#include "example.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <thread>
#include <sstream>
#include <iostream>
#include <iomanip>

int main(int argc, char * argv[]) try
{
    rs::context ctx;

    // Configure and start our device
    for(int i=0; i<ctx.get_device_count(); ++i)
    {
        rs::device dev = ctx.get_device(i);
        std::cout << "Starting " << dev.get_name() << "... ";
        dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
        dev.enable_stream(rs::stream::color, rs::preset::best_quality);
        dev.start();
        std::cout << "done." << std::endl;
    }

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Multi-Camera Example";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    font font;
    if (auto f = find_file("examples/assets/Roboto-Bold.ttf", 3))
    {
        font = ttf_create(f);
        fclose(f);
    }
    else throw std::runtime_error("Unable to open examples/assets/Roboto-Bold.ttf");

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();
        for(int i=0; i<ctx.get_device_count(); ++i)
        {
            ctx.get_device(i).wait_for_frames(RS_ALL_STREAM_BITS);
        }

        // Draw the images
        glClear(GL_COLOR_BUFFER_BIT);
        glPushMatrix();
        glOrtho(0, 1280, 960, 0, -1, +1);
        glPixelZoom(1, -1);
        int x=0;
        for(int i=0; i<ctx.get_device_count(); ++i)
        {
            auto dev = ctx.get_device(i);
            const auto c = dev.get_stream_intrinsics(rs::stream::color), d = dev.get_stream_intrinsics(rs::stream::depth);
            const int width = std::max(c.image_size.x, d.image_size.x);

            glRasterPos2i(x + (width - c.image_size.x)/2, 0);
            glDrawPixels(c.image_size.x, c.image_size.y, GL_RGB, GL_UNSIGNED_BYTE, dev.get_frame_data(rs::stream::color));

            glRasterPos2i(x + (width - d.image_size.x)/2, c.image_size.y);
            draw_depth_histogram(reinterpret_cast<const uint16_t *>(dev.get_frame_data(rs::stream::depth)), d.image_size.x, d.image_size.y);

            ttf_print(&font, x+(width-ttf_len(&font, dev.get_name()))/2, 24, dev.get_name());
            x += width;
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