#include <librealsense/rs.hpp>
#include "example.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

GLuint tex;
font font;

void draw_stream(rs::device & dev, rs::stream stream, int rx, int ry, int rw, int rh)
{
    if(!dev.is_stream_enabled(stream)) return;

    const rs::intrinsics intrin = dev.get_stream_intrinsics(stream);     
    glBindTexture(GL_TEXTURE_2D, tex);
    static uint8_t rgb_image[640*480*3];
    switch(dev.get_stream_format(stream))
    {
    case rs::format::z16:
        make_depth_histogram(rgb_image, reinterpret_cast<const uint16_t *>(dev.get_frame_data(stream)), intrin.width(), intrin.height());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, rgb_image);
        break;
    case rs::format::yuyv: // Display YUYV by showing the luminance channel and packing chrominance into ignored alpha channel
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, dev.get_frame_data(stream)); 
        break;
    case rs::format::rgb8: case rs::format::bgr8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, dev.get_frame_data(stream));
        break;
    case rs::format::rgba8: case rs::format::bgra8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, intrin.width(), intrin.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, dev.get_frame_data(stream));
        break;
    case rs::format::y8:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, dev.get_frame_data(stream));
        break;
    case rs::format::y16:
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, intrin.width(), intrin.height(), 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, dev.get_frame_data(stream));
        break;
    }

    float w = rw, h = rw * intrin.height() / intrin.width();
    if(h > rh)
    {
        float scale = rh/h;
        w *= scale;
        h *= scale;
    }
    float x0 = rx + (rw - w)/2, x1 = x0 + w, y0 = ry + (rh - h)/2, y1 = y0 + h;

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBegin(GL_QUADS);
    glTexCoord2f(0,0); glVertex2f(x0,y0);
    glTexCoord2f(1,0); glVertex2f(x1,y0);
    glTexCoord2f(1,1); glVertex2f(x1,y1);
    glTexCoord2f(0,1); glVertex2f(x0,y1);
    glEnd();

    std::ostringstream ss; ss << stream << ": " << intrin.width() << " x " << intrin.height() << " " << dev.get_stream_format(stream) << " (" << dev.get_stream_framerate(stream) << ")";
    ttf_print(&font, rx+8, ry+16, ss.str().c_str());
}

int main(int argc, char * argv[]) try
{
    rs::context ctx;
    if(ctx.get_device_count() < 1) throw std::runtime_error("No device detected. Is it plugged in?");

    // Configure our device
    rs::device dev = ctx.get_device(0);
    dev.enable_stream(rs::stream::color, rs::preset::largest_image);
    dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
    dev.enable_stream(rs::stream::infrared, 0, 0, rs::format::any, 0);
    try { dev.enable_stream(rs::stream::infrared2, 0, 0, rs::format::any, 0); } catch(...) {}

    // Compute field of view for each enabled stream
    for(int i = 0; i < RS_STREAM_COUNT; ++i)
    {
        auto stream = rs::stream(i);
        if(!dev.is_stream_enabled(stream)) continue;
        auto intrin = dev.get_stream_intrinsics(stream);
        std::cout << "Capturing " << stream << " at " << intrin.width() << " x " << intrin.height();
        std::cout << std::setprecision(1) << std::fixed << ", fov = " << intrin.hfov() << " x " << intrin.vfov() << (intrin.distorted() ? " (distorted)" : " (rectilinear)") << std::endl;
    }
    
    // Start our device
    dev.start();

    // Try setting some R200-specific settings
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    try {
        dev.set_option(rs::option::r200_lr_auto_exposure_enabled, 1);
    }  catch(...) {}

    // Report the status of each supported option
    for(int i = 0; i < RS_OPTION_COUNT; ++i)
    {
        auto option = rs::option(i);
        if(dev.supports_option(option))
        {
            std::cout << "Option " << option << ": ";
            try { std::cout << dev.get_option(option) << std::endl; }
            catch(const std::exception & e) { std::cout << e.what() << std::endl; }
        }
    }

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Capture Example (" << dev.get_name() << ")";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
        
    // Load our truetype font
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
        dev.wait_for_frames();
        std::cout << dev.get_frame_timestamp(rs::stream::depth) << " " << dev.get_frame_timestamp(rs::stream::color) << std::endl;

        // Clear the framebuffer
        int w,h;
        glfwGetWindowSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the images        
        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);
        draw_stream(dev, rs::stream::color, 0, 0, w/2, h/2);
        draw_stream(dev, rs::stream::depth, w/2, 0, w-w/2, h/2);
        draw_stream(dev, rs::stream::infrared, 0, h/2, w/2, h-h/2);
        draw_stream(dev, rs::stream::infrared2, w/2, h/2, w-w/2, h-h/2);
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