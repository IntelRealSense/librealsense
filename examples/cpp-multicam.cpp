#include <librealsense/rs.hpp>
#include "example.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

int main(int argc, char * argv[]) try
{
    rs::context ctx;

    // Configure and start our camera
    for(int i=0; i<ctx.get_camera_count(); ++i)
    {
        rs::camera cam = ctx.get_camera(i);
        std::cout << "Starting " << cam.get_name() << "... ";
        cam.enable_stream_preset(RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY);
        cam.enable_stream_preset(RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY);
        cam.start_capture();
        std::cout << "done." << std::endl;
    }

    // Open a GLFW window
	glfwInit();
    std::ostringstream ss; ss << "CPP Multi-Camera Example";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

    // Load our truetype font
    font font;
    if(auto f = fopen("../../examples/assets/Roboto-Bold.ttf", "rb"))
    {
        font = ttf_create(f);
        fclose(f);
    }
    else throw std::runtime_error("Unable to open examples/assets/Roboto-Bold.ttf");

	while (!glfwWindowShouldClose(win))
	{
        // Wait for new images
		glfwPollEvents();
        for(int i=0; i<ctx.get_camera_count(); ++i)
        {
            ctx.get_camera(i).wait_all_streams();
        }

        // Draw the images
		glClear(GL_COLOR_BUFFER_BIT);
        glPushMatrix();
        glOrtho(0, 1280, 960, 0, -1, +1);
        glPixelZoom(1, -1);
        int x=0;
        for(int i=0; i<ctx.get_camera_count(); ++i)
        {
            auto cam = ctx.get_camera(i);
            const auto c = cam.get_stream_intrinsics(RS_STREAM_COLOR), d = cam.get_stream_intrinsics(RS_STREAM_DEPTH);
            const int width = std::max(c.image_size[0], d.image_size[0]);

            glRasterPos2i(x + (width - c.image_size[0])/2, 0);
            glDrawPixels(c.image_size[0], c.image_size[1], GL_RGB, GL_UNSIGNED_BYTE, cam.get_image_pixels(RS_STREAM_COLOR));

            glRasterPos2i(x + (width - d.image_size[0])/2, c.image_size[1]);
            draw_depth_histogram(reinterpret_cast<const uint16_t *>(cam.get_image_pixels(RS_STREAM_DEPTH)), d.image_size[0], d.image_size[1]);

            ttf_print(&font, x+(width-ttf_len(&font, cam.get_name()))/2, 24, cam.get_name());
            x += width;
        }

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
