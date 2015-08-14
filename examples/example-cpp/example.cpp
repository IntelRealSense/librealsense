#include <librealsense/rs.hpp>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

float compute_fov(int image_size, float focal_length, float principal_point)
{
	return (atan2f(principal_point + 0.5f, focal_length) + atan2f(image_size - principal_point - 0.5f, focal_length)) * 180.0f / (float)M_PI;
}

int main(int argc, char * argv[]) try
{
	rs::camera cam;
	rs::context ctx;
	for (int i = 0; i < ctx.get_camera_count(); ++i)
	{
		std::cout << "Found camera at index " << i << std::endl;

		cam = ctx.get_camera(i);
		cam.enable_stream(RS_STREAM_DEPTH);
		cam.enable_stream(RS_STREAM_RGB);
		cam.configure_streams();

		float hfov = compute_fov(
			cam.get_stream_property_i(RS_STREAM_DEPTH, RS_IMAGE_SIZE_X),
			cam.get_stream_property_f(RS_STREAM_DEPTH, RS_FOCAL_LENGTH_X),
			cam.get_stream_property_f(RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_X));
		float vfov = compute_fov(
			cam.get_stream_property_i(RS_STREAM_DEPTH, RS_IMAGE_SIZE_Y),
			cam.get_stream_property_f(RS_STREAM_DEPTH, RS_FOCAL_LENGTH_Y),
			cam.get_stream_property_f(RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_Y));
		std::cout << "Computed FOV " << hfov << " " << vfov << std::endl;

        cam.start_stream_preset(RS_STREAM_DEPTH, RS_STREAM_PRESET_BEST_QUALITY);
        cam.start_stream_preset(RS_STREAM_RGB, RS_STREAM_PRESET_BEST_QUALITY);
	}
	if (!cam) throw std::runtime_error("No camera detected. Is it plugged in?");

	glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 480, "LibRealSense CPP Example", 0, 0);
	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();

		glfwMakeContextCurrent(win);
		glClear(GL_COLOR_BUFFER_BIT);
		glPixelZoom(1, -1);

		glRasterPos2f(-1, 1);
		glPixelTransferf(GL_RED_SCALE, 1);
        glDrawPixels(cam.get_stream_property_i(RS_STREAM_RGB, RS_IMAGE_SIZE_X),
                     cam.get_stream_property_i(RS_STREAM_RGB, RS_IMAGE_SIZE_Y),
                     GL_RGB, GL_UNSIGNED_BYTE, cam.get_color_image());

		glRasterPos2f(0, 1);
		glPixelTransferf(GL_RED_SCALE, 30);
        glDrawPixels(cam.get_stream_property_i(RS_STREAM_DEPTH, RS_IMAGE_SIZE_X),
                     cam.get_stream_property_i(RS_STREAM_DEPTH, RS_IMAGE_SIZE_Y),
                     GL_RED, GL_UNSIGNED_SHORT, cam.get_depth_image());

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
