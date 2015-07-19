#include <thread>
#include <chrono>
#include <vector>
#include <stdint.h>
#include <mutex>
#include <iostream>
#include "Util.h"

#include "libuvc/libuvc.h"

#include "glfw3.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#define GLFW_EXPOSE_NATIVE_NSGL
#include "glfw3native.h"

#include <OpenGL/gl3.h>

#include "CameraContext.h"
#include "CameraHeader.h"
#include "CameraSPI.h"
#include "CameraXU.h"
#include "GfxUtil.h"

GLFWwindow * window;

GLuint g_IvCamLocation;
GLuint g_DS4CamLocation;
GLuint textureHandle;

uint16_t g_IvImg[640 * 480];
uint16_t g_DsImg[628 * 469];

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

/*
void rgbCallback(uvc_frame_t *frame, void *ptr)
{
    static int frameCount;
    
    std::cout << "rgb_frame_count: " << frameCount << std::endl;
    
    std::vector<uint8_t> colorImg(frame->width * frame->height * 3); // YUY2 = 16 in -> 24 out
    convert_yuyv_rgb((const uint8_t *)frame->data, frame->width, frame->height, colorImg.data());
    memcpy(g_rgbImage, colorImg.data(), 640 * 480 * 3);
    
    frameCount++;
}
 */

void ivCallback(uvc_frame_t *frame, void *ptr)
{
    static int frameCount;
    
    std::cout << "iv framecount: " << frameCount << std::endl;
    //printf("frame dims %i x %i \n", frame->width, frame->height);
    
    memcpy(g_IvImg, frame->data, (frame->width * frame->height) * 2); // w * h * 2
    
    frameCount++;
}

void dsCallback(uvc_frame_t *frame, void *ptr)
{
    static int frameCount;
    
    std::cout << "ds framecount: " << frameCount << std::endl;
    //printf("frame dims %i x %i \n", frame->width, frame->height);
    
    memcpy(g_DsImg, frame->data, frame->width * (frame->height - 1) * 2); // w * h * 2
    
    frameCount++;
}

uvc_device_handle_t *g_IvHandle = nullptr;
uvc_device_handle_t *g_DsHandle = nullptr;

uvc_stream_ctrl_t g_IvCtrl;
uvc_stream_ctrl_t g_DsCtrl;

struct CamParams
{
    uvc_frame_format format;
    int width;
    int height;
    int fps;
    uvc_frame_callback_t * cb;
};

// ivcam = 0x0a66
// UVC_FRAME_FORMAT_INVI, 640, 480, 60
void CameraCaptureInit(uvc_context_t *ctx, uint16_t product_id, uvc_device_handle_t * devh, uvc_stream_ctrl_t &ctrl, CamParams params, int devNum = 1)
{
    uvc_device_t *uvcCameraDevice;
    
    uvc_error_t result;
    
    // Locates the first attached UVC device, stores in uvcCameraDevice
    result = uvc_find_device(ctx, &uvcCameraDevice, 0, product_id, NULL);
    
    if (result < 0)
    {
        std::cout << "================================================================================= \n";
        std::cout << "Couldn't find camera with product id: " << product_id << std::endl;
        std::cout << "================================================================================= \n";
        return;
        //throw std::runtime_error("Find Device Failed");
    }
    
    result = uvc_open2(uvcCameraDevice, &devh, devNum);

    if (result < 0)
    {
        uvc_perror(result, "uvc_open");
        return;
        //throw std::runtime_error("Open camera_handle Failed");
    }
    
    uvc_error_t status = uvc_get_stream_ctrl_format_size(devh, &ctrl, params.format, params.width, params.height, params.fps);

    if (status < 0)
    {
        uvc_perror(status, "set_format_size");
        //throw std::runtime_error("Open camera_handle Failed");
    }
    
    uvc_print_stream_ctrl(&ctrl, stderr);
    
    /* Print out a message containing all the information that libuvc
     * knows about the device */
    uvc_print_diag(devh, stderr);
    
    for (int i = 0; i < 10; ++i)
    {
        std::cout << "Fuck" << std::endl;
        SetStreamIntent(devh, 5);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
 
    
    for (int i = 0; i < 100; ++i)
    {
        
        uvc_error_t startStreamResult = uvc_start_streaming(devh, &ctrl, params.cb, nullptr, 0);
        
        for (int i = 0; i < 10; ++i)
        {
            std::cout << "Fuck" << std::endl;
            SetStreamIntent(devh, 5);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (startStreamResult < 0)
        {
            uvc_perror(startStreamResult, "start_stream");
            continue;
        }
        else {
            break;
        }
        
    }
    
}

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
    
    // Initialize overall context...
    uvc_error_t contextInitStatus;
    
    uvc_context_t *ctx;
    
    contextInitStatus = uvc_init(&ctx, NULL);

    if (contextInitStatus < 0)
    {
        uvc_perror(contextInitStatus, "uvc_init");
        throw std::runtime_error("UVC Init Failed");
    }
    
    // SPARK UP THE CAMERAS!
    // ---------------------
    
    //CamParams ivParams = {UVC_FRAME_FORMAT_INVR, 640, 480, 60, ivCallback};
    
    CamParams ivParams = {UVC_FRAME_FORMAT_YUYV, 640, 480, 0, ivCallback};
    CamParams dsParams = {UVC_FRAME_FORMAT_Z16, 628, 469, 0, dsCallback};
    
    //CameraCaptureInit(ctx, 0x0a66, g_IvHandle, g_IvCtrl, ivParams); // This is depth for ivCam
    
    CameraCaptureInit(ctx, 0x0a80, g_DsHandle, g_DsCtrl, dsParams, 1); // This is Depth for DS
    CameraCaptureInit(ctx, 0x0a80, g_IvHandle, g_IvCtrl, ivParams, 2); // This is RGB for DS
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    //SetStreamIntent(g_DsHandle, g_streamIntent);
    
    while(!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        glClearColor(0.15, 0.15, 0.15, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwMakeContextCurrent(window);
        
        //std::cout << "depth_stream_enabled? " << getStatus(g_depthHandle) << std::endl;
        //std::cout << "color_stream_enabled? " << getStatus(g_colorHandle) << std::endl;
        
        //if (getStatus(g_DsHandle) == 4)
        //{
        //    //SetStreamIntent(g_DsHandle, g_streamIntent);
        //}
    
        auto drawTex = [fullscreenTextureProg, quad_vertexbuffer](GLuint texId, void * pixels, int width, int height, GLint fmt, GLenum type)
        {
            
            CHECK_GL_ERROR();
            
            glUseProgram(fullscreenTextureProg);
            
            // Bind our texture in Texture Unit 0
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, type, pixels);
            glUniform1i(textureHandle, 0); // Set our "u_image" sampler to user Texture Unit 0
            
            CHECK_GL_ERROR();
            
            // 1st attribute buffer : vertices
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
            glVertexAttribPointer(
                                  0,                  // attribute 0 (layout location 1)
                                  3,                  // size
                                  GL_FLOAT,           // type
                                  GL_FALSE,           // normalized?
                                  0,                  // stride
                                  (void*)0            // array buffer offset
                                  );
            
            CHECK_GL_ERROR();
            
            // Draw the triangles !
            glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
            
            glDisableVertexAttribArray(0);
            
            glUseProgram(0);
            
        };
        
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        
        
        glViewport(0, 0, width, height);
        
        static uint8_t remappedIv[640 * 480 * 3];
        ConvertDepthToRGBUsingHistogram(remappedIv, g_IvImg, 640, 480, 0.3f, 0.825f);
        drawTex(g_IvCamLocation, remappedIv, 640, 480, GL_RGB, GL_UNSIGNED_BYTE);
        

        glViewport(width / 2, 0, width, height);
        static uint8_t remappedDS[628 * 469 * 3];
        ConvertDepthToRGBUsingHistogram(remappedDS, g_DsImg, 628, 469, 0.1f, 0.625f);
        drawTex(g_DS4CamLocation, remappedDS, 628, 469, GL_RGB, GL_UNSIGNED_BYTE);
        
        frameCount++;
        
        glfwSwapBuffers(window);
        CHECK_GL_ERROR();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        
    }
    
    glfwTerminate();
    return 0;
    
}