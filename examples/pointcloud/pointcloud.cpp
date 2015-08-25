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

#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>

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
        cam.enable_stream_preset(RS_INFRARED, RS_BEST_QUALITY);
        cam.enable_stream_preset(RS_DEPTH, RS_BEST_QUALITY);
        cam.enable_stream_preset(RS_COLOR, RS_BEST_QUALITY);
        cam.start_streaming();
	}
	if (!cam) throw std::runtime_error("No camera detected. Is it plugged in?");
    const auto depth_intrin = cam.get_stream_intrinsics(RS_DEPTH);
    struct state { float yaw, pitch; double lastX, lastY; bool ml; int tex_stream = RS_COLOR; } app_state = {};

	glfwInit();
    GLFWwindow * win = glfwCreateWindow(640, 480, "RealSense Point Cloud", 0, 0);
	glfwSetWindowUserPointer(win, &app_state);
        
	glfwSetMouseButtonCallback(win, [](GLFWwindow * win, int button, int action, int mods)
	{
		auto s = (state *)glfwGetWindowUserPointer(win);
		if(button == GLFW_MOUSE_BUTTON_LEFT) s->ml = action == GLFW_PRESS;
        if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) s->tex_stream = s->tex_stream == RS_COLOR ? RS_INFRARED : RS_COLOR;
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
        
    glfwSetKeyCallback(win, [](GLFWwindow * win, int key, int, int action, int)
    {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, 1);
    });

	glfwMakeContextCurrent(win);
    // glfwSwapInterval(0); // Use this if testing > 60 fps modes

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

    int frames = 0; float time = 0, fps = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(win))
	{
		glfwPollEvents();

		int width, height;
		glfwGetWindowSize(win, &width, &height);

        cam.wait_all_streams();

        auto t1 = std::chrono::high_resolution_clock::now();
        time += std::chrono::duration<float>(t1-t0).count();
        t0 = t1;
        ++frames;
        if(time > 0.5f)
        {
            fps = frames / time;
            frames = 0;
            time = 0;
        }

        glViewport(0, 0, width, height);
        glClearColor(0.0f, 116/255.0f, 197/255.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto tex_intrin = cam.get_stream_intrinsics(app_state.tex_stream);
        auto extrin = cam.get_stream_extrinsics(RS_DEPTH, app_state.tex_stream);
        
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_intrin.image_size[0], tex_intrin.image_size[1], 0,
                     app_state.tex_stream == RS_COLOR ? GL_RGB : GL_LUMINANCE, GL_UNSIGNED_BYTE, cam.get_image_pixels(app_state.tex_stream));
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

		glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);

        glPointSize((float)width/640);
        glBegin(GL_POINTS);
        auto depth = cam.get_depth_image();
        float scale = cam.get_depth_scale();
		for(int y=0; y<depth_intrin.image_size[1]; ++y)
		{
			for(int x=0; x<depth_intrin.image_size[0]; ++x)
			{
                if(auto d = *depth++)
				{
                    float depth_pixel[2] = {static_cast<float>(x),static_cast<float>(y)}, depth_point[3], tex_point[3], tex_pixel[2];
                    rs_deproject_pixel_to_point(depth_point, depth_intrin, depth_pixel, d*scale);
                    rs_transform_point_to_point(tex_point, extrin, depth_point);
                    rs_project_point_to_pixel(tex_pixel, tex_intrin, tex_point);

                    glTexCoord2f(tex_pixel[0] / tex_intrin.image_size[0], tex_pixel[1] / tex_intrin.image_size[1]);
                    glVertex3fv(depth_point);
				}
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
        std::ostringstream ss; ss << fps << " FPS";
        ttf_print(cdata, 20, 40, ss.str().c_str());
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
