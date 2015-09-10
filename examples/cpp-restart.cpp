#include <librealsense/rs.hpp>
#include "example.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

font font;

float compute_fov(int image_size, float focal_length, float principal_point)
{
    return (atan2f(principal_point + 0.5f, focal_length) + atan2f(image_size - principal_point - 0.5f, focal_length)) * 180.0f / (float)M_PI;
}

void draw_stream(rs::device & dev, rs::stream stream, int x, int y)
{
    if(!dev.is_stream_enabled(stream)) return;

    const rs::intrinsics intrin = dev.get_stream_intrinsics(stream);
    const rs::format format = dev.get_stream_format(stream);
    const int width = intrin.image_size.x, height = intrin.image_size.y, fps = dev.get_stream_framerate(stream);
    const void * pixels = dev.get_frame_data(stream);

    glRasterPos2i(x + (640 - intrin.image_size.x)/2, y + (480 - intrin.image_size.y)/2);
    glPixelZoom(1, -1);
    switch(format)
    {
    case rs::format::z16:
        draw_depth_histogram(reinterpret_cast<const uint16_t *>(pixels), width, height);
        break;
    case rs::format::yuyv:
        glDrawPixels(width, height, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pixels); // Display YUYV by showing the luminance channel and packing chrominance into ignored alpha channel
        break;
    case rs::format::rgb8: case rs::format::bgr8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        break;
    case rs::format::rgba8: case rs::format::bgra8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
        glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        break;
    case rs::format::y8:
        glDrawPixels(width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
        break;
    case rs::format::y16:
        glDrawPixels(width, height, GL_LUMINANCE, GL_UNSIGNED_SHORT, pixels);
        break;
    }

    std::ostringstream ss; ss << stream << ": " << intrin.image_size.x << " x " << intrin.image_size.y << " " << format;
    ttf_print(&font, x+8, y+16, ss.str().c_str());
}

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

    for(int i=0; i<8; ++i)
    {
        switch(i)
        {
        case 0:
            device.enable_stream(rs::stream::depth, rs::preset::best_quality);
            device.enable_stream(rs::stream::color, rs::preset::best_quality);
            break;
        case 1:
            device.enable_stream(rs::stream::depth, rs::preset::largest_image);
            device.enable_stream(rs::stream::color, rs::preset::best_quality);
            break;
        case 2:
            device.enable_stream(rs::stream::depth, rs::preset::best_quality);
            device.enable_stream(rs::stream::color, rs::preset::best_quality);
            device.enable_stream(rs::stream::infrared, rs::preset::best_quality);
            break;
        case 3:
            device.enable_stream(rs::stream::depth, rs::preset::largest_image);
            device.enable_stream(rs::stream::color, rs::preset::best_quality);
            device.enable_stream(rs::stream::infrared, rs::preset::largest_image);
            break;
        case 4:
            device.enable_stream(rs::stream::depth, rs::preset::best_quality);
            device.enable_stream(rs::stream::color, rs::preset::best_quality);
            device.enable_stream(rs::stream::infrared, rs::preset::best_quality);
            device.enable_stream(rs::stream::infrared2, rs::preset::best_quality);
            break;
        case 5:
            device.enable_stream(rs::stream::depth, rs::preset::largest_image);
            device.enable_stream(rs::stream::color, rs::preset::best_quality);
            device.enable_stream(rs::stream::infrared, rs::preset::largest_image);
            device.enable_stream(rs::stream::infrared2, rs::preset::largest_image);
            break;
        case 6:
            device.enable_stream(rs::stream::depth, rs::preset::best_quality);
            device.disable_stream(rs::stream::color);
            device.disable_stream(rs::stream::infrared);
            device.disable_stream(rs::stream::infrared2);
            break;
        case 7:
            device.disable_stream(rs::stream::depth);
            device.enable_stream(rs::stream::color, rs::preset::best_quality);
            break;
        }

        device.start();
        for(int j=0; j<120; ++j)
        {
            // Wait for new images
            glfwPollEvents();
            if(glfwWindowShouldClose(win)) goto done;
            device.wait_for_frames();

            // Draw the images
            glClear(GL_COLOR_BUFFER_BIT);
            glPushMatrix();
            glOrtho(0, 1280, 960, 0, -1, +1);
            draw_stream(device, rs::stream::color, 0, 0);
            draw_stream(device, rs::stream::depth, 640, 0);
            draw_stream(device, rs::stream::infrared, 0, 480);
            draw_stream(device, rs::stream::infrared2, 640, 480);
            glPopMatrix();
            glfwSwapBuffers(win);
        }
        device.stop();
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