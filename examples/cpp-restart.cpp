#include <librealsense/rs.hpp>
#include "example.hpp"

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

font font;
texture_buffer buffers[RS_STREAM_COUNT];

int main(int argc, char * argv[]) try
{
    // Obtain a RealSense device
    rs::context ctx;
    if(ctx.get_device_count() < 1) throw std::runtime_error("No device detected. Is it plugged in?");
    rs::device device = ctx.get_device(0);

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Restart Example (" << device.get_name() << ")";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);
        
    // Load our truetype font
    if (auto f = find_file("examples/assets/Roboto-Bold.ttf", 3))
    {
        font = ttf_create(f);
        fclose(f);
    }
    else throw std::runtime_error("Unable to open examples/assets/Roboto-Bold.ttf");

    for(int i=0; i<20; ++i)
    {
        try
        {
            if(device.is_streaming()) device.stop();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            for(int j=0; j<RS_STREAM_COUNT; ++j)
            {
                auto s = (rs::stream)j;
                if(device.is_stream_enabled(s)) device.disable_stream(s);
            }

            switch(i)
            {
            case 0:
                device.enable_stream(rs::stream::color, 640, 480, rs::format::yuyv, 60);
                break;
            case 1:
                device.enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 60);
                break;
            case 2:
                device.enable_stream(rs::stream::color, 640, 480, rs::format::bgr8, 60);
                break;
            case 3:
                device.enable_stream(rs::stream::color, 640, 480, rs::format::rgba8, 60);
                break;
            case 4:
                device.enable_stream(rs::stream::color, 640, 480, rs::format::bgra8, 60);
                break;
            case 5:
                device.enable_stream(rs::stream::color, 1920, 1080, rs::format::rgb8, 60);
                break;
            case 6:
                device.enable_stream(rs::stream::depth, rs::preset::largest_image);
                break;
            case 7:
                device.enable_stream(rs::stream::depth, 480, 360, rs::format::z16, 60);
                break;
            case 8:
                device.enable_stream(rs::stream::depth, 320, 240, rs::format::z16, 60);
                break;
            case 9:
                device.enable_stream(rs::stream::infrared, rs::preset::largest_image);
                break;
            case 10:
                device.enable_stream(rs::stream::infrared, 492, 372, rs::format::y8, 60);
                break;
            case 11:
                device.enable_stream(rs::stream::infrared, 320, 240, rs::format::y8, 60);
                break;
            case 12:
                device.enable_stream(rs::stream::infrared, 0, 0, rs::format::y16, 60);
                break;
            case 13:
                device.enable_stream(rs::stream::infrared, 0, 0, rs::format::y8, 60);
                device.enable_stream(rs::stream::infrared2, 0, 0, rs::format::y8, 60);
                break;
            case 14:
                device.enable_stream(rs::stream::infrared, 0, 0, rs::format::y16, 60);
                device.enable_stream(rs::stream::infrared2, 0, 0, rs::format::y16, 60);
                break;
            case 15:
                device.enable_stream(rs::stream::depth, rs::preset::best_quality);
                device.enable_stream(rs::stream::infrared, 0, 0, rs::format::y8, 0);
                break;
            case 16:
                device.enable_stream(rs::stream::depth, rs::preset::best_quality);
                device.enable_stream(rs::stream::infrared, 0, 0, rs::format::y16, 0);
                break;
            case 17:
                device.enable_stream(rs::stream::depth, rs::preset::best_quality);
                device.enable_stream(rs::stream::color, rs::preset::best_quality);
                break;
            case 18:
                device.enable_stream(rs::stream::depth, rs::preset::best_quality);
                device.enable_stream(rs::stream::color, rs::preset::best_quality);
                device.enable_stream(rs::stream::infrared, rs::preset::best_quality);
                break;
            case 19:
                device.enable_stream(rs::stream::depth, rs::preset::best_quality);
                device.enable_stream(rs::stream::color, rs::preset::best_quality);
                device.enable_stream(rs::stream::infrared, rs::preset::best_quality);
                device.enable_stream(rs::stream::infrared2, rs::preset::best_quality);
                break;
            }

            device.start();
            for(int j=0; j<120; ++j)
            {
                // Wait for new images
                glfwPollEvents();
                if(glfwWindowShouldClose(win)) goto done;
                device.wait_for_frames();

                // Clear the framebuffer
                int w,h;
                glfwGetWindowSize(win, &w, &h);
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                // Draw the images
                glPushMatrix();
                glfwGetWindowSize(win, &w, &h);
                glOrtho(0, w, h, 0, -1, +1);
                buffers[0].show(device, rs::stream::color, 0, 0, w/2, h/2, font);
                buffers[1].show(device, rs::stream::depth, w/2, 0, w-w/2, h/2, font);
                buffers[2].show(device, rs::stream::infrared, 0, h/2, w/2, h-h/2, font);
                buffers[3].show(device, rs::stream::infrared2, w/2, h/2, w-w/2, h-h/2, font);
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
