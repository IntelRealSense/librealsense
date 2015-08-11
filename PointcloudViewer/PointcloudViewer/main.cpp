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

#if defined(ANDROID)
    #define PLATFORM_ANDROID 1
#endif

#include <thread>
#include <chrono>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <iostream>

#if defined(PLATFORM_WINDOWS)

#include <GL\glew.h>
#include <GLFW\glfw3.h>

#elif defined(PLATFORM_OSX)

#include "glfw3.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#include "glfw3native.h"
#include <OpenGL/gl3.h>

#endif

#include "GfxUtil.h"

#include "librealsense/rs.hpp"

GLFWwindow * window;

std::string vertShader = R"(#version 330 core
    layout(location = 0) in vec3 position;
    out vec2 texCoord;
    void main()
    {
        gl_Position = vec4(position, 1);
        texCoord = (position.xy + vec2(1,1)) / 2.0;
    }
)";

std::string fragShader = R"(#version 330 core
    uniform sampler2D u_image;
    in vec2 texCoord;
    out vec3 color;
    void main()
    {
        color = texture(u_image, texCoord.st * vec2(1.0, -1.0)).rgb;
    }
)";

static const GLfloat g_quad_vertex_buffer_data[] =
{
    1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    1.0f,  -1.0f, 0.0f,
    1.0f,  -1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f,
};

GLuint rgbTextureHandle;
GLuint depthTextureHandle;
GLuint imageUniformHandle;

// Compute field of view angles in degrees from rectified intrinsics
inline float GetAsymmetricFieldOfView(int imageSize, float focalLength, float principalPoint)
{ 
	return (atan2f(principalPoint + 0.5f, focalLength) + atan2f(imageSize - principalPoint - 0.5f, focalLength)) * 180.0f / (float)M_PI;
}

int main(int argc, const char * argv[]) try
{
    uint64_t frameCount = 0;
    
    if (!glfwInit())
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }
    
#if defined(PLATFORM_OSX)
    glfwWindowHint(GLFW_SAMPLES, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    
    window = glfwCreateWindow(1280, 480, "R200 Pointcloud", NULL, NULL);
    
    if (!window)
    {
        std::cout << "Failed to open GLFW window\n" << std::endl;
        glfwTerminate();
        std::exit( EXIT_FAILURE );
    }
    
    glfwMakeContextCurrent(window);
#if defined(PLATFORM_WINDOWS)
	glewInit();
#endif
    
    glfwSetWindowSizeCallback(window,
                              [](GLFWwindow* window, int width, int height)
                              {
                                  glViewport(0, 0, width, height);
                              });
    
    glfwSetKeyCallback (window,
                        [](GLFWwindow * window, int key, int, int action, int mods)
                        {
                           if (action == GLFW_PRESS)
                           {
                               if (key == GLFW_KEY_ESCAPE)
                               {
                                   glfwSetWindowShouldClose(window, 1);
                               }
                           }
                        });
    
	rs::context realsenseContext;
	rs::camera camera;

	// Init RealSense R200 Camera -------------------------------------------  
    if (realsenseContext.get_camera_count() == 0)
    { 
        std::cout << "Error: no cameras detected. Is it plugged in?" << std::endl;
		return EXIT_FAILURE;
    }

	for (int i = 0; i < realsenseContext.get_camera_count(); ++i)
    {
        std::cout << "Found Camera At Index: " << i << std::endl;
                
		camera = realsenseContext.get_camera(i);

        camera.enable_stream(RS_STREAM_DEPTH);
        camera.enable_stream(RS_STREAM_RGB);
        camera.configure_streams();
             
		float hFov = GetAsymmetricFieldOfView(
			camera.get_stream_property_i(RS_STREAM_DEPTH, RS_IMAGE_SIZE_X),
			camera.get_stream_property_f(RS_STREAM_DEPTH, RS_FOCAL_LENGTH_X),
			camera.get_stream_property_f(RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_X));
		float vFov = GetAsymmetricFieldOfView(
			camera.get_stream_property_i(RS_STREAM_DEPTH, RS_IMAGE_SIZE_Y),
			camera.get_stream_property_f(RS_STREAM_DEPTH, RS_FOCAL_LENGTH_Y),
			camera.get_stream_property_f(RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_Y));
        std::cout << "Computed FoV: " << hFov << " x " << vFov << std::endl;
                                
		camera.start_stream(RS_STREAM_DEPTH, 628, 469, 0, RS_FRAME_FORMAT_Z16);
		camera.start_stream(RS_STREAM_RGB, 640, 480, 30, RS_FRAME_FORMAT_YUYV);
    }
    // ----------------------------------------------------------------
    
    // Remapped colors...
    rgbTextureHandle = CreateTexture(640, 480, GL_RGB);     // Normal RGB
    depthTextureHandle = CreateTexture(628, 468, GL_RGB);   // Depth to RGB remap
    
    GLuint quad_VertexArrayID;
    glGenVertexArrays(1, &quad_VertexArrayID);
    glBindVertexArray(quad_VertexArrayID);
    
    GLuint quadVBO;
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
    
    // Create and compile our GLSL program from the shaders
    GLuint fullscreenTextureProg = CreateGLProgram(vertShader, fragShader);
    imageUniformHandle = glGetUniformLocation(fullscreenTextureProg, "u_image");
    
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        glClearColor(0.15f, 0.15f, 0.15f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwMakeContextCurrent(window);
    
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        
        if (camera && camera.is_streaming())
        {
            glViewport(0, 0, width, height);
			auto depthImage = camera.get_depth_image();
            static uint8_t depthColoredHistogram[628 * 468 * 3];
            ConvertDepthToRGBUsingHistogram(depthColoredHistogram, depthImage, 628, 468, 0.1f, 0.625f);
            drawTexture(fullscreenTextureProg, quadVBO, imageUniformHandle, depthTextureHandle, depthColoredHistogram, 628, 468, GL_RGB, GL_UNSIGNED_BYTE);
            
            glViewport(width / 2, 0, width, height);
			auto colorImage = camera.get_color_image();
            drawTexture(fullscreenTextureProg, quadVBO, imageUniformHandle, rgbTextureHandle, colorImage, 640, 480, GL_RGB, GL_UNSIGNED_BYTE);
        }
        
        frameCount++;
        
        glfwSwapBuffers(window);
        CHECK_GL_ERROR();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 16 fps
        
    }
    
    glfwTerminate();
    return 0;
    
}
catch (const std::exception & e)
{
	std::cerr << "Caught exception: " << e.what() << std::endl;
	return EXIT_FAILURE;
}