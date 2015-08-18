#include <librealsense/rs.hpp>

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

FILE * find_file(std::string path, int levels)
{
    for(int i=0; i<=levels; ++i)
    {
        if(auto f = fopen(path.c_str(), "rb")) return f;
        path = "../" + path;
    }
    return nullptr;
}

void ttf_print(stbtt_bakedchar cdata[], float x, float y, const char *text)
{
   // assume orthographic projection with units = screen pixels, origin at top left
   glBegin(GL_QUADS);
   while (*text) {
      if (*text >= 32 && *text < 128) {
         stbtt_aligned_quad q;
         stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
         glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
         glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
         glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
         glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
      }
      ++text;
   }
   glEnd();
}

float ttf_len(stbtt_bakedchar cdata[], const char *text)
{
    float x=0, y=0;
   while (*text) {
      if (*text >= 32 && *text < 128) {
         stbtt_aligned_quad q;
         stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
      }
      ++text;
   }
   return x;
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

        cam.start_stream_preset(RS_STREAM_DEPTH, RS_STREAM_PRESET_BEST_QUALITY);
        cam.start_stream_preset(RS_STREAM_RGB, RS_STREAM_PRESET_BEST_QUALITY);
	}
	if (!cam) throw std::runtime_error("No camera detected. Is it plugged in?");
	const auto depth_intrin = cam.get_stream_intrinsics(RS_STREAM_DEPTH), color_intrin = cam.get_stream_intrinsics(RS_STREAM_RGB);
	const auto extrin = cam.get_stream_extrinsics(RS_STREAM_DEPTH, RS_STREAM_RGB);

    struct state { float yaw, pitch; double lastX, lastY; bool ml; } app_state = {};

	glfwInit();
    GLFWwindow * win = glfwCreateWindow(640, 480, "RealSense Point Cloud", 0, 0);
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

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    GLuint ftex;
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyph
    if(auto f = find_file("examples/assets/Roboto-Bold.ttf", 3))
    {
        fseek(f, 0, SEEK_END);
        std::vector<uint8_t> ttf_buffer(ftell(f));
        fseek(f, 0, SEEK_SET);
        fread(ttf_buffer.data(), 1, ttf_buffer.size(), f);

        unsigned char temp_bitmap[512*512];
        stbtt_BakeFontBitmap(ttf_buffer.data(),0, 20.0, temp_bitmap, 512,512, 32,96, cdata); // no guarantee this fits!

        glGenTextures(1, &ftex);
        glBindTexture(GL_TEXTURE_2D, ftex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }
    else throw std::runtime_error("Unable to open examples/assets/Roboto-Bold.ttf");

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glPopAttrib();

	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();

		int width, height;
		glfwGetWindowSize(win, &width, &height);
		
        auto depth = cam.get_depth_image();

        glViewport(0, 0, width, height);
        glClearColor(0.0f, 116/255.0f, 197/255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cam.get_stream_property_i(RS_STREAM_RGB, RS_IMAGE_SIZE_X),
                     cam.get_stream_property_i(RS_STREAM_RGB, RS_IMAGE_SIZE_Y), 0, GL_RGB, GL_UNSIGNED_BYTE, cam.get_color_image());
        glMatrixMode(GL_PROJECTION);
		glPushMatrix();
        gluPerspective(60, (float)width/height, 0.01f, 20.0f);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
		gluLookAt(0,0,0, 0,0,1, 0,-1,0);
        glTranslatef(0,0,+0.5f);

        glRotatef(app_state.pitch, 1, 0, 0);
        glRotatef(app_state.yaw, 0, 1, 0);
        glTranslatef(0,0,-0.5f);

        float scale = rs_get_depth_scale(cam.get_handle(), NULL);
		glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);

        glPointSize((float)width/640);
        glBegin(GL_POINTS);

        auto v = rs_get_vertex_image(cam.get_handle(), 0);
		for(int y=0; y<depth_intrin.image_size[1]; ++y)
		{
			for(int x=0; x<depth_intrin.image_size[0]; ++x)
			{
                if(v[2])
				{
                    float color_point[3], color_pixel[2];
                    rs_transform_point_to_point(v, extrin, color_point);
                    rs_project_point_to_pixel(color_point, color_intrin, color_pixel);
                    glTexCoord2f(color_pixel[0] / color_intrin.image_size[0], color_pixel[1] / color_intrin.image_size[1]);
                    glVertex3f(v[0]*0.001f, v[1]*0.001f, v[2]*0.001f);
				}
                v += 3;
			}
		}
		glEnd();
		glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glOrtho(0, width, height, 0, -1, +1);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, ftex);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ttf_print(cdata, (width-ttf_len(cdata, cam.get_name()))/2, height-20.0f, cam.get_name());
        glPopMatrix();
        glPopAttrib();

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
