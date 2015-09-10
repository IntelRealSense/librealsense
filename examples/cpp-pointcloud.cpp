#include <librealsense/rs.hpp>
#include <librealsense/rsutil.h>
#include "example.h"

#include <chrono>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

struct state { double yaw, pitch, lastX, lastY; bool ml; std::vector<rs::stream> tex_streams; int index; rs::device * dev; };

int main(int argc, char * argv[]) try
{
    rs::device dev;
    rs::context ctx;
    for (int i = 0; i < ctx.get_device_count(); ++i)
    {
        dev = ctx.get_device(i);
        dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
        dev.enable_stream(rs::stream::color, rs::preset::best_quality);
        try 
        { 
            dev.enable_stream(rs::stream::infrared, rs::preset::best_quality);
            dev.enable_stream(rs::stream::infrared2, rs::preset::best_quality); 
        } catch(...) {}
        dev.start();
    }
    if (!dev) throw std::runtime_error("No device detected. Is it plugged in?");
        
    state app_state = {0, 0, 0, 0, false, {rs::stream::color, rs::stream::infrared}, 0, &dev};
    if(dev.is_stream_enabled(rs::stream::infrared)) app_state.tex_streams.push_back(rs::stream::infrared);
    if(dev.is_stream_enabled(rs::stream::infrared2)) app_state.tex_streams.push_back(rs::stream::infrared2);
    
    glfwInit();
    std::ostringstream ss; ss << "CPP Point Cloud Example (" << dev.get_name() << ")";
    GLFWwindow * win = glfwCreateWindow(640, 480, ss.str().c_str(), 0, 0);
    glfwSetWindowUserPointer(win, &app_state);
        
    glfwSetMouseButtonCallback(win, [](GLFWwindow * win, int button, int action, int mods)
    {
        auto s = (state *)glfwGetWindowUserPointer(win);
        if(button == GLFW_MOUSE_BUTTON_LEFT) s->ml = action == GLFW_PRESS;
        if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) s->index = (s->index+1) % s->tex_streams.size();
    });
        
    glfwSetCursorPosCallback(win, [](GLFWwindow * win, double x, double y)
    {
        auto s = (state *)glfwGetWindowUserPointer(win);
        if(s->ml)
        {
            s->yaw -= (x - s->lastX);
            s->yaw = std::max(s->yaw, -120.0);
            s->yaw = std::min(s->yaw, +120.0);
            s->pitch += (y - s->lastY);
            s->pitch = std::max(s->pitch, -80.0);
            s->pitch = std::min(s->pitch, +80.0);
        }
        s->lastX = x;
        s->lastY = y;
    });
        
    glfwSetKeyCallback(win, [](GLFWwindow * win, int key, int scancode, int action, int mods)
    {
        auto s = (state *)glfwGetWindowUserPointer(win);
        if (action == GLFW_PRESS)
        {
            if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(win, 1);
            else if (key == GLFW_KEY_F1) s->dev->start();
            else if (key == GLFW_KEY_F2) s->dev->stop();
        }
    });

    glfwMakeContextCurrent(win);
    // glfwSwapInterval(0); // Use this if testing > 60 fps modes

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    font font;
    if (auto f = find_file("examples/assets/Roboto-Bold.ttf", 3))
    {
        font = ttf_create(f);
        fclose(f);
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
        if(dev.is_streaming()) dev.wait_for_frames();

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

        const rs::stream tex_stream = app_state.tex_streams[app_state.index];
        const float depth_scale = dev.get_depth_scale();
        const rs::extrinsics extrin = dev.get_extrinsics(rs::stream::depth, tex_stream);
        const rs::intrinsics depth_intrin = dev.get_stream_intrinsics(rs::stream::depth);
        const rs::intrinsics tex_intrin = dev.get_stream_intrinsics(tex_stream);
        bool identical = depth_intrin == tex_intrin && extrin.is_identity();

        int width, height;
        glfwGetFramebufferSize(win, &width, &height);
       
        glPushAttrib(GL_ALL_ATTRIB_BITS);

        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        switch(dev.get_stream_format(tex_stream))
        {
        case rs::format::rgb8: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_intrin.image_size.x, tex_intrin.image_size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, dev.get_frame_data(tex_stream)); break;
        case rs::format::y8:   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_intrin.image_size.x, tex_intrin.image_size.y, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, dev.get_frame_data(tex_stream)); break;
        case rs::format::y16:  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_intrin.image_size.x, tex_intrin.image_size.y, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, dev.get_frame_data(tex_stream)); break;
        }

        glViewport(0, 0, width, height);
        glClearColor(52.0f/255, 72.f/255, 94.0f/255.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        gluPerspective(60, (float)width/height, 0.01f, 20.0f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        gluLookAt(0,0,0, 0,0,1, 0,-1,0);

        glTranslatef(0,0,+0.5f);
        glRotated(app_state.pitch, 1, 0, 0);
        glRotated(app_state.yaw, 0, 1, 0);
        glTranslatef(0,0,-0.5f);

        glPointSize((float)width/640);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_POINTS);
        auto depth = reinterpret_cast<const uint16_t *>(dev.get_frame_data(rs::stream::depth));
        
        for(int y=0; y<depth_intrin.image_size.y; ++y)
        {
            for(int x=0; x<depth_intrin.image_size.x; ++x)
            {
                if(auto d = *depth++)
                {
                    rs::float3 point = depth_intrin.deproject({static_cast<float>(x), static_cast<float>(y)}, d*depth_scale);
                    if(identical) glTexCoord2f((x+0.5f)/depth_intrin.image_size.x, (y+0.5f)/depth_intrin.image_size.y);
                    else
                    {
                        rs::float2 tex_pixel = tex_intrin.project(extrin.transform(point));
                        glTexCoord2f((tex_pixel.x+0.5f) / tex_intrin.image_size.x, (tex_pixel.y+0.5f) / tex_intrin.image_size.y);
                    }
                    glVertex3fv(&point.x);
                }
            }
        }
        glEnd();
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();

        glfwGetWindowSize(win, &width, &height);
        
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glOrtho(0, width, height, 0, -1, +1);
        
        std::ostringstream ss; ss << dev.get_name() << " (" << app_state.tex_streams[app_state.index] << ")";
        ttf_print(&font, (width-ttf_len(&font, ss.str().c_str()))/2, height-20.0f, ss.str().c_str());

        ss.str(""); ss << fps << " FPS";
        ttf_print(&font, 20, 40, ss.str().c_str());
        glPopMatrix();

        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}