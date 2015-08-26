#include <librealsense/rs.hpp>
#include "../example.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <iomanip>>

font font;

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

    ttf_print(&font, x+8, y+16, rs_get_stream_name(stream, 0));
}

int main(int argc, char * argv[]) try
{
	rs::camera cam;
	rs::context ctx;
	for (int i = 0; i < ctx.get_camera_count(); ++i)
	{
		std::cout << "Found camera at index " << i << std::endl;

		cam = ctx.get_camera(i);

        cam.enable_stream_preset(RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY);
        cam.enable_stream_preset(RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY);
        cam.enable_stream_preset(RS_STREAM_INFRARED, RS_PRESET_BEST_QUALITY);
        cam.enable_stream_preset(RS_STREAM_INFRARED_2, RS_PRESET_BEST_QUALITY);
        cam.start_capture();

        for(int j = RS_STREAM_BEGIN_RANGE; j <= RS_STREAM_END_RANGE; ++j)
        {
            if(!cam.is_stream_enabled((rs::stream)j)) continue;
            auto intrin = cam.get_stream_intrinsics((rs::stream)j);
            float hfov = compute_fov(intrin.image_size[0], intrin.focal_length[0], intrin.principal_point[0]);
            float vfov = compute_fov(intrin.image_size[1], intrin.focal_length[1], intrin.principal_point[1]);
            std::cout << "Capturing " << rs::get_name((rs::stream)j) << " at " << intrin.image_size[0] << " x " << intrin.image_size[1];
            std::cout << std::setprecision(1) << std::fixed << ", fov = " << hfov << " x " << vfov << std::endl;
        }
	}
	if (!cam) throw std::runtime_error("No camera detected. Is it plugged in?");

	glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 960, "LibRealSense CPP Example", 0, 0);
    glfwMakeContextCurrent(win);
    font = ttf_create(fopen("../../examples/assets/Roboto-Bold.ttf", "rb"));

	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();
        cam.wait_all_streams();

		glClear(GL_COLOR_BUFFER_BIT);
        glPushMatrix();
        glOrtho(0, 1280, 960, 0, -1, +1);

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
