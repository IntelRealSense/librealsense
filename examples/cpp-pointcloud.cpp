#include <librealsense/rs.hpp>
#include <librealsense/rsutil.h>
#include "example.h"

#include <chrono>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

FILE * find_file(std::string path, int levels)
{
    for(int i=0; i<=levels; ++i)
    {
        if(auto f = fopen(path.c_str(), "rb")) return f;
        path = "../" + path;
    }
    return nullptr;
}

struct state { double yaw, pitch, lastX, lastY; bool ml; std::vector<rs_stream> tex_streams; int index; rs::camera * cam; };
int main(int argc, char * argv[]) try
{
    rs::camera cam;
    rs::context ctx;
    for (int i = 0; i < ctx.get_camera_count(); ++i)
    {
        std::cout << "Found camera at index " << i << std::endl;

        cam = ctx.get_camera(i);
        cam.enable_stream_preset(RS_STREAM_DEPTH, RS_PRESET_BEST_QUALITY);
        cam.enable_stream_preset(RS_STREAM_COLOR, RS_PRESET_BEST_QUALITY);
        cam.enable_stream_preset(RS_STREAM_INFRARED, RS_PRESET_BEST_QUALITY);
        try { cam.enable_stream_preset(RS_STREAM_INFRARED_2, RS_PRESET_BEST_QUALITY); } catch(...) {}
        cam.start_capture();
    }
    if (!cam) throw std::runtime_error("No camera detected. Is it plugged in?");
        
    state app_state = {0, 0, 0, 0, false, {RS_STREAM_COLOR, RS_STREAM_INFRARED}, 0, &cam};
    if(cam.is_stream_enabled(RS_STREAM_INFRARED_2)) app_state.tex_streams.push_back(RS_STREAM_INFRARED_2);
    
    glfwInit();
    std::ostringstream ss; ss << "CPP Point Cloud Example (" << cam.get_name() << ")";
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
            else if (key == GLFW_KEY_F1) s->cam->start_capture();
            else if (key == GLFW_KEY_F2) s->cam->stop_capture();
        }
    });

    glfwMakeContextCurrent(win);
    // glfwSwapInterval(0); // Use this if testing > 60 fps modes

    glPushAttrib(GL_ALL_ATTRIB_BITS);

    font font;
    if(auto f = find_file("examples/assets/Roboto-Bold.ttf", 3))
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
        if(cam.is_capturing()) cam.wait_all_streams();

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
        const float depth_scale = cam.get_depth_scale();
        const rs::intrinsics depth_intrin = cam.get_stream_intrinsics(RS_STREAM_DEPTH);
        const rs::intrinsics tex_intrin = cam.get_stream_intrinsics(tex_stream);
        const rs::extrinsics extrin = cam.get_stream_extrinsics(RS_STREAM_DEPTH, tex_stream);
        bool identical = memcmp(&depth_intrin, &tex_intrin, sizeof(rs::intrinsics)) == 0
            && extrin.rotation[0] == 1 && extrin.rotation[4] == 1 && extrin.rotation[8] == 1
            && extrin.translation[0] == 0 && extrin.translation[1] == 0 && extrin.translation[2] == 0;

        int width, height;
        glfwGetFramebufferSize(win, &width, &height);
       
        glPushAttrib(GL_ALL_ATTRIB_BITS);

        glBindTexture(GL_TEXTURE_2D, tex);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex_intrin.image_size[0], tex_intrin.image_size[1], 0,
                     tex_stream == RS_STREAM_COLOR ? GL_RGB : GL_LUMINANCE, GL_UNSIGNED_BYTE, cam.get_image_pixels(tex_stream));

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
        auto depth = reinterpret_cast<const uint16_t *>(cam.get_image_pixels(RS_STREAM_DEPTH));
        
        for(int y=0; y<depth_intrin.image_size[1]; ++y)
        {
            for(int x=0; x<depth_intrin.image_size[0]; ++x)
            {
                if(auto d = *depth++)
                {
                    float depth_pixel[2] = {static_cast<float>(x),static_cast<float>(y)}, depth_point[3], tex_point[3], tex_pixel[2];
                    rs_deproject_pixel_to_point(depth_point, &depth_intrin, depth_pixel, d*depth_scale);
                    if(identical) glTexCoord2f((x+0.5f)/depth_intrin.image_size[0], (y+0.5f)/depth_intrin.image_size[1]);
                    else
                    {
                        rs_transform_point_to_point(tex_point, &extrin, depth_point);
                        rs_project_point_to_pixel(tex_pixel, &tex_intrin, tex_point);
                        glTexCoord2f((tex_pixel[0]+0.5f) / tex_intrin.image_size[0], (tex_pixel[1]+0.5f) / tex_intrin.image_size[1]);
                    }
                    glVertex3fv(depth_point);
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
        
        std::ostringstream ss; ss << cam.get_name() << " (" << app_state.tex_streams[app_state.index] << ")";
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
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
