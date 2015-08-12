#include <librealsense/rs.hpp>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <iostream>

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

		cam.start_stream(RS_STREAM_DEPTH, 628, 469, 0, RS_FRAME_FORMAT_Z16);
		cam.start_stream(RS_STREAM_RGB, 640, 480, 30, RS_FRAME_FORMAT_YUYV);
	}
	if (!cam) throw std::runtime_error("No camera detected. Is it plugged in?");
	const auto depth_intrin = cam.get_stream_intrinsics(RS_STREAM_DEPTH), color_intrin = cam.get_stream_intrinsics(RS_STREAM_RGB);
	const auto extrin = cam.get_stream_extrinsics(RS_STREAM_DEPTH, RS_STREAM_RGB);

	struct state { float yaw, pitch; double lastX, lastY; bool ml; } app_state = {0,0,false};

	glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 720, "LibRealSense Point Cloud Example", 0, 0);
	glfwSetWindowUserPointer(win, &app_state);
	glfwSetMouseButtonCallback(win, [](GLFWwindow * win, int button, int action, int mods)
	{
		auto s = (state *)glfwGetWindowUserPointer(win);
		if(button == GLFW_MOUSE_BUTTON_LEFT) s->ml = action == GLFW_PRESS;
	});
	glfwSetCursorPosCallback(win, [](GLFWwindow * win, double x, double y)
	{
		auto s = (state *)glfwGetWindowUserPointer(win);
		if(s->ml)
		{
			s->yaw -= (x - s->lastX);
			s->yaw = std::max(s->yaw, -120.0f);
			s->yaw = std::min(s->yaw, +120.0f);
			s->pitch += (y - s->lastY);
			s->pitch = std::max(s->pitch, -80.0f);
			s->pitch = std::min(s->pitch, +80.0f);
		}
		s->lastX = x;
		s->lastY = y;
	});
	glfwMakeContextCurrent(win);

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();

		int width, height;
		glfwGetWindowSize(win, &width, &height);
		
		auto depth = cam.get_depth_image();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, color_intrin.image_size[0], color_intrin.image_size[1], 0, GL_RGB, GL_UNSIGNED_BYTE, cam.get_color_image());

		glClearColor(0.3f,0.3f,0.3f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPushMatrix();
		gluPerspective(60, (float)width/height, 10.0f, 2000.0f);
		gluLookAt(0,0,0, 0,0,1, 0,-1,0);
		glTranslatef(0,0,500);

		glRotatef(app_state.pitch, 1, 0, 0);
		glRotatef(app_state.yaw, 0, 1, 0);
		glTranslatef(0,0,-500);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glBegin(GL_POINTS);
		for(int y=0; y<depth_intrin.image_size[1]; ++y)
		{
			for(int x=0; x<depth_intrin.image_size[0]; ++x)
			{
				if(auto d = *depth++)
				{
					const float pixel[] = {x,y};
					const auto point = depth_intrin.deproject_from_rectified(pixel, d);
					glTexCoord2fv(color_intrin.project_to_texcoord(extrin.transform(point.data()).data()).data());
					glVertex3fv(point.data());
				}
			}
		}
		glEnd();

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
