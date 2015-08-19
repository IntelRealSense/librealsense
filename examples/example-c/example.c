#include <librealsense/rs.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct rs_error * error;
void check_error()
{
	if (error)
	{
		fprintf(stderr, "error calling %s(...):\n  %s\n", rs_get_failed_function(error), rs_get_error_message(error));
		rs_free_error(error);
		exit(EXIT_FAILURE);
	}
}

float compute_fov(int image_size, float focal_length, float principal_point)
{
	return (atan2f(principal_point + 0.5f, focal_length) + atan2f(image_size - principal_point - 0.5f, focal_length)) * 180.0f / (float)M_PI;
}

int main(int argc, char * argv[])
{
	struct rs_context * ctx;
	struct rs_camera * cam;
	float hfov, vfov;
	GLFWwindow * win;
    int i;

	ctx = rs_create_context(RS_API_VERSION, &error); check_error();
    for (i = 0; i < rs_get_camera_count(ctx, NULL); ++i)
	{
		printf("Found camera at index %d\n", i);

		cam = rs_get_camera(ctx, i, &error); check_error();
        rs_enable_stream(cam, RS_STREAM_DEPTH, &error); check_error();
        rs_enable_stream(cam, RS_STREAM_RGB, &error); check_error();
		rs_configure_streams(cam, &error); check_error();

		hfov = compute_fov(
			rs_get_stream_property_i(cam, RS_STREAM_DEPTH, RS_IMAGE_SIZE_X, NULL),
			rs_get_stream_property_f(cam, RS_STREAM_DEPTH, RS_FOCAL_LENGTH_X, NULL),
			rs_get_stream_property_f(cam, RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_X, NULL));
		vfov = compute_fov(
			rs_get_stream_property_i(cam, RS_STREAM_DEPTH, RS_IMAGE_SIZE_Y, NULL),
			rs_get_stream_property_f(cam, RS_STREAM_DEPTH, RS_FOCAL_LENGTH_Y, NULL),
			rs_get_stream_property_f(cam, RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_Y, NULL));
		printf("Computed FOV %f %f\n", hfov, vfov);

        rs_start_stream_preset(cam, RS_STREAM_DEPTH, RS_STREAM_PRESET_BEST_QUALITY, &error); check_error();
        rs_start_stream_preset(cam, RS_STREAM_RGB, RS_STREAM_PRESET_BEST_QUALITY, &error); check_error();
	}
	if (!cam)
	{
		fprintf(stderr, "No camera detected. Is it plugged in?\n");
		return -1;
	}

	glfwInit();
	win = glfwCreateWindow(1280, 480, "LibRealSense C Example", 0, 0);
	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();
        rs_wait_all_streams(cam, &error); check_error();

		glfwMakeContextCurrent(win);
		glClear(GL_COLOR_BUFFER_BIT);
		glPixelZoom(1, -1);

        glRasterPos2f(-1, 1);
		glPixelTransferf(GL_RED_SCALE, 1);
        glDrawPixels(rs_get_stream_property_i(cam, RS_STREAM_RGB, RS_IMAGE_SIZE_X, NULL),
                     rs_get_stream_property_i(cam, RS_STREAM_RGB, RS_IMAGE_SIZE_Y, NULL),
                     GL_RGB, GL_UNSIGNED_BYTE, rs_get_color_image(cam, &error)); check_error();

        glRasterPos2f(0, 1);
		glPixelTransferf(GL_RED_SCALE, 30);
        glDrawPixels(rs_get_stream_property_i(cam, RS_STREAM_DEPTH, RS_IMAGE_SIZE_X, NULL),
                     rs_get_stream_property_i(cam, RS_STREAM_DEPTH, RS_IMAGE_SIZE_Y, NULL),
                     GL_RED, GL_UNSIGNED_SHORT, rs_get_depth_image(cam, &error)); check_error();

		glfwSwapBuffers(win);
	}

	glfwDestroyWindow(win);
	glfwTerminate();

	rs_delete_context(ctx, &error); check_error();
	return 0;
}
