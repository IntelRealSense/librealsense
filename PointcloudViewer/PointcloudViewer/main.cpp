#include <thread>
#include <chrono>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <iostream>
//#include "Util.h"



#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL


#if __APPLE__
#include <OpenGL/gl3.h>
#include "glfw3.h"
#include "glfw3native.h"
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#endif

#include "GfxUtil.h"

#include "librealsense/CameraContext.h"
#include "librealsense/UVCCamera.h"
#include "librealsense/R200/R200.h"
#include "librealsense/F200/F200.h"

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

using namespace rs;
using namespace r200;
using namespace f200;

std::unique_ptr<CameraContext> realsenseContext;
//R200Camera * camera;
F200Camera * camera;

int main(int argc, const char * argv[])
{
    uint64_t frameCount = 0;
    
    if (!glfwInit())
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }
    
    glfwWindowHint(GLFW_SAMPLES, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(1280, 480, "R200 Pointcloud", NULL, NULL);
    
    if (!window)
    {
        std::cout << "Failed to open GLFW window\n" << std::endl;
        glfwTerminate();
        std::exit( EXIT_FAILURE );
    }
    
    glfwMakeContextCurrent(window);
    
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
        realsenseContext.reset(new CameraContext());
    
        auto cameraList = realsenseContext->cameras;
        
        if (cameraList.size() == 0)
        { 
            std::cout << "Error: no cameras detected. Is it plugged in?" << std::endl;
        }
        else
        {
            for (auto cam : realsenseContext->cameras)
            {
                std::cout << "Found Camera At Index: " << cam->GetCameraIndex() << std::endl;
                
                // R200
                /*
                camera = static_cast<R200Camera*>(cam.get());

                cam->EnableStream(STREAM_DEPTH);
                cam->EnableStream(STREAM_RGB);
                
                cam->ConfigureStreams();
             
                auto zIntrin = camera->GetRectifiedIntrinsicsZ();
                float hFov, vFov;
                GetFieldOfView(zIntrin, hFov, vFov);
                std::cout << "Computed FoV: " << hFov << " x " << vFov << std::endl;

                StreamConfiguration depthConfig = {628, 469, 0, UVC_FRAME_FORMAT_Z16};
                StreamConfiguration colorConfig = {640, 480, 30, UVC_FRAME_FORMAT_YUYV};
                
                cam->StartStream(STREAM_DEPTH, depthConfig);
                cam->StartStream(STREAM_RGB, colorConfig);
                */
                // F200

                camera = static_cast<F200Camera*>(cam.get());

                cam->EnableStream(STREAM_DEPTH);
                //cam->EnableStream(STREAM_RGB);

                cam->ConfigureStreams();

                StreamConfiguration depthConfig = {640, 480, 0, UVC_FRAME_FORMAT_INVZ};
                //StreamConfiguration colorConfig = {640, 480, 30, UVC_FRAME_FORMAT_YUYV};

                cam->StartStream(STREAM_DEPTH, depthConfig);
                //cam->StartStream(STREAM_RGB, colorConfig);

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

        
        if (camera && camera->IsStreaming())
        {
            glViewport(0, 0, width, height);
            auto depthImage = realsenseContext->cameras[0]->GetDepthImage();
            static uint8_t depthColoredHistogram[628 * 469 * 3];
            ConvertDepthToRGBUsingHistogram(depthColoredHistogram, depthImage, 628, 469, 0.1f, 0.625f);
            drawTexture(fullscreenTextureProg, quadVBO, imageUniformHandle, depthTextureHandle, depthColoredHistogram, 628, 469, GL_RGB, GL_UNSIGNED_BYTE);
            
            //glViewport(width / 2, 0, width, height);
            //auto colorImage = realsenseContext->cameras[0]->GetColorImage();
            //drawTexture(fullscreenTextureProg, quadVBO, imageUniformHandle, rgbTextureHandle, colorImage, 640, 480, GL_RGB, GL_UNSIGNED_BYTE);
        }
        
        frameCount++;
        
        glfwSwapBuffers(window);
        CHECK_GL_ERROR();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // 30 fps
        
    }
    
    glfwTerminate();
    return 0;
    
}
