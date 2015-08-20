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
    struct rs_intrinsics color_intrin, depth_intrin;
	float hfov, vfov;
	GLFWwindow * win;
    int i;

	ctx = rs_create_context(RS_API_VERSION, &error); check_error();
    for (i = 0; i < rs_get_camera_count(ctx, NULL); ++i)
	{
		printf("Found camera at index %d\n", i);

		cam = rs_get_camera(ctx, i, &error); check_error();
        rs_enable_stream_preset(cam, RS_DEPTH, RS_BEST_QUALITY, &error); check_error();
        rs_enable_stream_preset(cam, RS_COLOR, RS_BEST_QUALITY, &error); check_error();
        rs_start_streaming(cam, &error); check_error();

        rs_get_stream_intrinsics(cam, RS_COLOR, &color_intrin, &error); check_error();
        rs_get_stream_intrinsics(cam, RS_DEPTH, &depth_intrin, &error); check_error();
        hfov = compute_fov(depth_intrin.image_size[0], depth_intrin.focal_length[0], depth_intrin.principal_point[0]);
        vfov = compute_fov(depth_intrin.image_size[1], depth_intrin.focal_length[1], depth_intrin.principal_point[1]);
		printf("Computed FOV %f %f\n", hfov, vfov);
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
        glDrawPixels(color_intrin.image_size[0], color_intrin.image_size[1], GL_RGB, GL_UNSIGNED_BYTE, rs_get_color_image(cam, &error)); check_error();

        glRasterPos2f(0, 1);
		glPixelTransferf(GL_RED_SCALE, 30);
        glDrawPixels(depth_intrin.image_size[0], depth_intrin.image_size[1], GL_RED, GL_UNSIGNED_SHORT, rs_get_depth_image(cam, &error)); check_error();

		glfwSwapBuffers(win);
	}

	glfwDestroyWindow(win);
	glfwTerminate();

	rs_delete_context(ctx, &error); check_error();
	return 0;
}
