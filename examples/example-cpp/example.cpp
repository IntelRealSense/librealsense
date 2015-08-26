///////////////
// Compilers //
///////////////

#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#endif

#if defined(__GNUC__)
#define COMPILER_GCC 1
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if defined(__clang__)
#define COMPILER_CLANG 1
#endif

///////////////
// Platforms //
///////////////

#if defined(WIN32) || defined(_WIN32)
#define PLATFORM_WINDOWS 1
#endif

#ifdef __APPLE__
#define PLATFORM_OSX 1
#endif

#if defined(__linux__)
#define PLATFORM_LINUX 1
#endif

#include <librealsense/rs.hpp>

#define GLFW_INCLUDE_GLU

#if defined(PLATFORM_OSX)

#include "glfw3.h"
#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#include "glfw3native.h"
#include <OpenGL/gl3.h>
#elif defined(PLATFORM_LINUX)
#include <GLFW/glfw3.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

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
    const void * pixels = cam.get_image_pixels(pixels);

    glRasterPos2i(x, y);
    switch(format)
    {
    case RS_FORMAT_Z16:
        glPixelTransferf(GL_RED_SCALE, 30);
        glDrawPixels(width, height, GL_RED, GL_UNSIGNED_SHORT, pixels);
        glPixelTransferf(GL_RED_SCALE, 1);
        break;
    case RS_FORMAT_Y8:
        glDrawPixels(width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
        break;
    case RS_FORMAT_RGB8:
        glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        break;
    }
}

int main(int argc, char * argv[]) try
{
	rs::camera cam;
	rs::context ctx;
	for (int i = 0; i < ctx.get_camera_count(); ++i)
	{
		std::cout << "Found camera at index " << i << std::endl;

		cam = ctx.get_camera(i);

        cam.enable_stream_preset(RS_STREAM_DEPTH, RS_BEST_QUALITY);
        cam.enable_stream_preset(RS_STREAM_COLOR, RS_BEST_QUALITY);
        //cam.enable_stream_preset(RS_STREAM_INFRARED, RS_BEST_QUALITY);
        //cam.enable_stream_preset(RS_STREAM_INFRARED_2, RS_BEST_QUALITY);
        cam.start_streaming();

        if(cam.is_stream_enabled(RS_DEPTH))
        {
            auto intrin = cam.get_stream_intrinsics(RS_DEPTH);
            float hfov = compute_fov(intrin.image_size[0], intrin.focal_length[0], intrin.principal_point[0]);
            float vfov = compute_fov(intrin.image_size[1], intrin.focal_length[1], intrin.principal_point[1]);
            std::cout << "Computed FOV " << hfov << " " << vfov << std::endl;
        }
	}
	if (!cam) throw std::runtime_error("No camera detected. Is it plugged in?");

	glfwInit();
    GLFWwindow * win = glfwCreateWindow(1600, 720, "LibRealSense CPP Example", 0, 0);
	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();
        cam.wait_all_streams();

		glfwMakeContextCurrent(win);
		glClear(GL_COLOR_BUFFER_BIT);
        glPushMatrix();
        glOrtho(0, 1600, 0, 720, -1, +1);
		glPixelZoom(1, -1);

        draw_stream(cam, RS_STREAM_COLOR, 0, 720);
        draw_stream(cam, RS_STREAM_DEPTH, 640, 720);
        draw_stream(cam, RS_STREAM_INFRARED, 640, 360);
        draw_stream(cam, RS_STREAM_INFRARED_2, 1120, 360);
        
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
