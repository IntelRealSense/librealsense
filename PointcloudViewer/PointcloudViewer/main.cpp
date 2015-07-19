#include <thread>
#include <chrono>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <iostream>
#include "Util.h"

#include "glfw3.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#include "glfw3native.h"

#include <OpenGL/gl3.h>

#include "GfxUtil.h"
#include "librealsense/Common.h"
#include "librealsense/CameraContext.h"

GLFWwindow * window;

GLuint g_IvCamLocation;
GLuint g_DS4CamLocation;
GLuint textureHandle;

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
    in vec2 texCoord;
    out vec3 color;
    uniform sampler2D u_image;
    void main()
    {
        color = texture(u_image, texCoord.st * vec2(1.0, -1.0)).rgb;
    }
)";


int main(int argc, const char * argv[])
{
    static int frameCount = 0;
    
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
    
    window = glfwCreateWindow(1280, 480, "DS4 & OSX", NULL, NULL);
    
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
    
    // Remapped colors...
    g_IvCamLocation = CreateTexture(640, 480, GL_RGB);
    g_DS4CamLocation = CreateTexture(628, 468, GL_RGB);
    
    // glfwSetWindowShouldClose(window, 1);
    
    // The fullscreen quad's FBO
    GLuint quad_VertexArrayID;
    glGenVertexArrays(1, &quad_VertexArrayID);
    glBindVertexArray(quad_VertexArrayID);
    
    static const GLfloat g_quad_vertex_buffer_data[] =
    {
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        1.0f,  -1.0f, 0.0f,
        1.0f,  -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
    };
    
    GLuint quad_vertexbuffer;
    glGenBuffers(1, &quad_vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
    
    // Create and compile our GLSL program from the shaders
    GLuint fullscreenTextureProg = CreateGLProgram(vertShader, fragShader);
    textureHandle = glGetUniformLocation(fullscreenTextureProg, "u_image");
    
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        glClearColor(0.15, 0.15, 0.15, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwMakeContextCurrent(window);
    
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);
        
        /*
        static uint8_t remappedIv[640 * 480 * 3];
        ConvertDepthToRGBUsingHistogram(remappedIv, g_IvImg, 640, 480, 0.3f, 0.825f);
        drawTex(g_IvCamLocation, remappedIv, 640, 480, GL_RGB, GL_UNSIGNED_BYTE);
        

        glViewport(width / 2, 0, width, height);
        static uint8_t remappedDS[628 * 469 * 3];
        ConvertDepthToRGBUsingHistogram(remappedDS, g_DsImg, 628, 469, 0.1f, 0.625f);
        drawTex(g_DS4CamLocation, remappedDS, 628, 469, GL_RGB, GL_UNSIGNED_BYTE);
         */
        
        frameCount++;
        
        glfwSwapBuffers(window);
        CHECK_GL_ERROR();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // 16 fps
        
    }
    
    glfwTerminate();
    return 0;
    
}