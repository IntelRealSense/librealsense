#include <librealsense/rs.hpp>
#include "example.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

font font;

FILE * find_file(std::string path, int levels)
{
    for(int i=0; i<=levels; ++i)
    {
        if(auto f = fopen(path.c_str(), "rb")) return f;
        path = "../" + path;
    }
    return nullptr;
}

float compute_fov(int image_size, float focal_length, float principal_point)
{
	return (atan2f(principal_point + 0.5f, focal_length) + atan2f(image_size - principal_point - 0.5f, focal_length)) * 180.0f / (float)M_PI;
}

void draw_stream(rs::camera & cam, rs::stream stream, int x, int y)
{
    if(!cam.is_stream_enabled(stream)) return;

    const rs::intrinsics intrin = cam.get_stream_intrinsics(stream);
    const int width = intrin.image_size[0], height = intrin.image_size[1];
    const rs::format format = cam.get_image_format(stream);
    const void * pixels = cam.get_image_pixels(stream);

    glRasterPos2i(x + (640 - intrin.image_size[0])/2, y + (480 - intrin.image_size[1])/2);
    glPixelZoom(1, -1);
    switch(format)
    {
    case RS_FORMAT_Z16:
        draw_depth_histogram(reinterpret_cast<const uint16_t *>(pixels), width, height);
        break;
    case RS_FORMAT_Y8:
        glDrawPixels(width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
        break;
    case RS_FORMAT_RGB8:
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        break;
    }

    std::ostringstream ss; ss << stream << ": " << intrin.image_size[0] << " x " << intrin.image_size[1] << " " << format;
    ttf_print(&font, x+8, y+16, ss.str().c_str());
}

int main(int argc, char * argv[]) try
{
    rs::context ctx;
    if(ctx.get_camera_count() < 1) throw std::runtime_error("No camera detected. Is it plugged in?");

    // Configure and start our camera
    rs::camera cam = ctx.get_camera(0);
    cam.enable_stream_preset(RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY);
    cam.enable_stream_preset(RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY);
    try {
        cam.enable_stream_preset(RS_STREAM_INFRARED, RS_PRESET_BEST_QUALITY);
        cam.enable_stream_preset(RS_STREAM_INFRARED_2, RS_PRESET_BEST_QUALITY);
    } catch(...) {}
    cam.start_capture();

    // Compute field of view for each enabled stream
    for(rs::stream stream = RS_STREAM_BEGIN_RANGE; stream <= RS_STREAM_END_RANGE; stream = (rs::stream)(stream+1))
    {
        if(!cam.is_stream_enabled(stream)) continue;
        auto intrin = cam.get_stream_intrinsics(stream);
        float hfov = compute_fov(intrin.image_size[0], intrin.focal_length[0], intrin.principal_point[0]);
        float vfov = compute_fov(intrin.image_size[1], intrin.focal_length[1], intrin.principal_point[1]);
        std::cout << "Capturing " << stream << " at " << intrin.image_size[0] << " x " << intrin.image_size[1];
        std::cout << std::setprecision(1) << std::fixed << ", fov = " << hfov << " x " << vfov << std::endl;
    }

    // Report the status of each supported option
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    for(rs::option option = RS_OPTION_BEGIN_RANGE; option <= RS_OPTION_END_RANGE; option = (rs::option)(option+1))
    {
        if(cam.supports_option(option))
        {
            std::cout << "Option " << option << ": ";
            try { std::cout << cam.get_option(option) << std::endl; }
            catch(const std::exception & e) { std::cout << e.what() << std::endl; }
        }
    }

    // Try setting some R200-specific settings
    try {
        cam.set_option(RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED, 1);
    }  catch(...) {}

    // Open a GLFW window
	glfwInit();
    const int height = cam.is_stream_enabled(RS_STREAM_INFRARED) || cam.is_stream_enabled(RS_STREAM_INFRARED_2) ? 960 : 480;
    std::ostringstream ss; ss << "CPP Capture Example (" << cam.get_name() << ")";
    GLFWwindow * win = glfwCreateWindow(1280, height, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);
        
    // Load our truetype font
    if(auto f = find_file("examples/assets/Roboto-Bold.ttf", 3))
    {
        font = ttf_create(f);
        fclose(f);
    }
    else throw std::runtime_error("Unable to open examples/assets/Roboto-Bold.ttf");

	while (!glfwWindowShouldClose(win))
	{
        // Wait for new images
		glfwPollEvents();
        cam.wait_all_streams();

        // Draw the images
		glClear(GL_COLOR_BUFFER_BIT);
        glPushMatrix();
        glOrtho(0, 1280, height, 0, -1, +1);
        draw_stream(cam, RS_STREAM_COLOR, 0, 0);
        draw_stream(cam, RS_STREAM_DEPTH, 640, 0);
        draw_stream(cam, RS_STREAM_INFRARED, 0, 480);
        draw_stream(cam, RS_STREAM_INFRARED_2, 640, 480);
        glPopMatrix();
		glfwSwapBuffers(win);
	}

	glfwDestroyWindow(win);
	glfwTerminate();
	return EXIT_SUCCESS;
}
catch (const std::exception & e)
{
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}
