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
        cam.enable_stream_preset(RS_DEPTH, RS_BEST_QUALITY);
        cam.enable_stream_preset(RS_COLOR, RS_BEST_QUALITY);
        cam.start_streaming();

        auto intrin = cam.get_stream_intrinsics(RS_DEPTH);
        float hfov = compute_fov(intrin.image_size[0], intrin.focal_length[0], intrin.principal_point[0]);
        float vfov = compute_fov(intrin.image_size[1], intrin.focal_length[1], intrin.principal_point[1]);
		std::cout << "Computed FOV " << hfov << " " << vfov << std::endl;
	}
	if (!cam) throw std::runtime_error("No camera detected. Is it plugged in?");

	glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 480, "LibRealSense CPP Example", 0, 0);
	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();
        cam.wait_all_streams();

		glfwMakeContextCurrent(win);
		glClear(GL_COLOR_BUFFER_BIT);
		glPixelZoom(1, -1);

        auto c = cam.get_stream_intrinsics(RS_COLOR);
		glRasterPos2f(-1, 1);
		glPixelTransferf(GL_RED_SCALE, 1);
        glDrawPixels(c.image_size[0], c.image_size[1], GL_RGB, GL_UNSIGNED_BYTE, cam.get_color_image());

        auto d = cam.get_stream_intrinsics(RS_DEPTH);
		glRasterPos2f(0, 1);
		glPixelTransferf(GL_RED_SCALE, 30);
        glDrawPixels(d.image_size[0], d.image_size[1], GL_RED, GL_UNSIGNED_SHORT, cam.get_depth_image());

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
