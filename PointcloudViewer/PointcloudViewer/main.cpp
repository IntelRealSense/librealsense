#include <thread>
#include <chrono>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <iostream>

#ifdef WIN32
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#elif __APPLE__

#include "glfw3.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#include "glfw3native.h"
#include <OpenGL/gl3.h>
#endif

#include "GfxUtil.h"

#include "librealsense/rs.h"

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

RScontext realsenseContext;
RScamera camera;

// Compute field of view angles in degrees from rectified intrinsics
inline float GetAsymmetricFieldOfView(int imageSize, float focalLength, float principalPoint)
{ 
	return (atan2(principalPoint + 0.5f, focalLength) + atan2(imageSize - principalPoint - 0.5f, focalLength)) * 180.0f / M_PI;
}

int main(int argc, const char * argv[]) try
{
    uint64_t frameCount = 0;
    
    if (!glfwInit())
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }
    
    //glfwWindowHint(GLFW_SAMPLES, 2);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(1280, 480, "R200 Pointcloud", NULL, NULL);
    
    if (!window)
    {
        std::cout << "Failed to open GLFW window\n" << std::endl;
        glfwTerminate();
        std::exit( EXIT_FAILURE );
    }
    
    glfwMakeContextCurrent(window);
#ifdef WIN32
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
    
    // Init RealSense R200 Camera -------------------------------------------
    {
		realsenseContext = rsCreateContext();
            
        if (rsGetCameraCount(realsenseContext) == 0)
        { 
            std::cout << "Error: no cameras detected. Is it plugged in?" << std::endl;
        }
        else
        {
			for (int i = 0; i < rsGetCameraCount(realsenseContext); ++i)
            {
                std::cout << "Found Camera At Index: " << i << std::endl;
                
				camera = rsGetCamera(realsenseContext, i);

                rsEnableStream(camera, RS_STREAM_DEPTH);
                rsEnableStream(camera, RS_STREAM_RGB);
                rsConfigureStreams(camera);
             
				float hFov = GetAsymmetricFieldOfView(
					rsGetStreamPropertyi(camera, RS_STREAM_DEPTH, RS_IMAGE_SIZE_X),
					rsGetStreamPropertyf(camera, RS_STREAM_DEPTH, RS_FOCAL_LENGTH_X),
					rsGetStreamPropertyf(camera, RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_X));
				float vFov = GetAsymmetricFieldOfView(
					rsGetStreamPropertyi(camera, RS_STREAM_DEPTH, RS_IMAGE_SIZE_Y),
					rsGetStreamPropertyf(camera, RS_STREAM_DEPTH, RS_FOCAL_LENGTH_Y),
					rsGetStreamPropertyf(camera, RS_STREAM_DEPTH, RS_PRINCIPAL_POINT_Y));
                std::cout << "Computed FoV: " << hFov << " x " << vFov << std::endl;
                                
				rsStartStream(camera, RS_STREAM_DEPTH, 628, 469, 0, RS_FRAME_FORMAT_Z16);
				rsStartStream(camera, RS_STREAM_RGB, 640, 480, 30, RS_FRAME_FORMAT_YUYV);
            }
        }
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
        
        glClearColor(0.15, 0.15, 0.15, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwMakeContextCurrent(window);
    
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        
        if (camera && rsIsStreaming(camera))
        {
            glViewport(0, 0, width, height);
			auto depthImage = rsGetDepthImage(camera);
            static uint8_t depthColoredHistogram[628 * 468 * 3];
            ConvertDepthToRGBUsingHistogram(depthColoredHistogram, depthImage, 628, 468, 0.1f, 0.625f);
            drawTexture(fullscreenTextureProg, quadVBO, imageUniformHandle, depthTextureHandle, depthColoredHistogram, 628, 468, GL_RGB, GL_UNSIGNED_BYTE);
            
            glViewport(width / 2, 0, width, height);
			auto colorImage = rsGetColorImage(camera);
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