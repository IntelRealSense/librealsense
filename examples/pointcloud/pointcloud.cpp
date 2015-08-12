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

	glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 720, "LibRealSense Point Cloud Example", 0, 0);
	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();

		glfwMakeContextCurrent(win);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glPushMatrix();
		gluPerspective(60, (float)1280/720, 10.0f, 2000.0f);

		auto depth_intrin = cam.get_stream_intrinsics(RS_STREAM_DEPTH), color_intrin = cam.get_stream_intrinsics(RS_STREAM_RGB);
		auto extrin = cam.get_stream_extrinsics(RS_STREAM_DEPTH, RS_STREAM_RGB);
		auto depth = cam.get_depth_image();
		auto color = cam.get_color_image();

		glBegin(GL_POINTS);
		for(int y=0; y<depth_intrin.image_size[1]; ++y)
		{
			for(int x=0; x<depth_intrin.image_size[0]; ++x)
			{
				float pixel[] = {x,y};
				auto d = *depth++;
				if(d)
				{
					if(x == 320 && y == 240)
					{
						int z = 5;
					}

					auto point = depth_intrin.deproject_from_rectified(pixel, d);
					auto texCoord = color_intrin.project(extrin.transform(point.data()).data());
					int s = (int)texCoord[0], t = (int)texCoord[1];
					if(s >= 0 && t >= 0 && s < color_intrin.image_size[0] && t < color_intrin.image_size[1])
					{
						glColor3ubv(color + (t*color_intrin.image_size[0] + s)*3);
					}
					else glColor3f(1,1,1);
					glVertex3f(point[0], -point[1], -point[2]);
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
