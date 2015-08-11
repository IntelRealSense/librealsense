#include <librealsense/rs.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

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

	ctx = rs_create_context(RS_API_VERSION, NULL);
	for (int i = 0; i < rs_get_camera_count(ctx, NULL); ++i)
	{
		printf("Found camera at index %d\n", i);

		cam = rs_get_camera(ctx, i, NULL);
		rs_enable_stream(cam, RS_STREAM_DEPTH, NULL);
		rs_enable_stream(cam, RS_STREAM_RGB, NULL);
		rs_configure_streams(cam, NULL);

		hfov = compute_fov(
			rs_get_stream_property_i(cam, RS_STREAM_DEPTH, RS_IMAGE_SIZE_X, NULL),
			rs_get_stream_property_f(cam, RS_STREAM_DEPTH, RS_FOCAL_LENGTH_X, NULL),
			rs_get_stream_property_f(cam, RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_X, NULL));
		vfov = compute_fov(
			rs_get_stream_property_i(cam, RS_STREAM_DEPTH, RS_IMAGE_SIZE_Y, NULL),
			rs_get_stream_property_f(cam, RS_STREAM_DEPTH, RS_FOCAL_LENGTH_Y, NULL),
			rs_get_stream_property_f(cam, RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_Y, NULL));
		printf("Computed FOV %f %f\n", hfov, vfov);

		rs_start_stream(cam, RS_STREAM_DEPTH, 628, 469, 0, RS_FRAME_FORMAT_Z16, NULL);
		rs_start_stream(cam, RS_STREAM_RGB, 640, 480, 30, RS_FRAME_FORMAT_YUYV, NULL);
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

		glfwMakeContextCurrent(win);
		glClear(GL_COLOR_BUFFER_BIT);
		glPixelZoom(1, -1);

		glRasterPos2f(-1, 1);
		glPixelTransferf(GL_RED_SCALE, 1);
		glDrawPixels(640, 480, GL_RGB, GL_UNSIGNED_BYTE, rs_get_color_image(cam, NULL));

		glRasterPos2f(0, 1);
		glPixelTransferf(GL_RED_SCALE, 30);
		glDrawPixels(628, 468, GL_RED, GL_UNSIGNED_SHORT, rs_get_depth_image(cam, NULL));

		glfwSwapBuffers(win);
	}

	glfwDestroyWindow(win);
	glfwTerminate();

	rs_delete_context(ctx, NULL);
	return 0;
}
