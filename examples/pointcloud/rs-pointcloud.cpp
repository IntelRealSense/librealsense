// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include "example.hpp"
#include <iostream>

#include <algorithm>
#include <fstream>

struct state { double yaw, pitch, last_x, last_y; bool ml; float offset_x, offset_y; };

void register_glfw_callbacks(window& app);

int main(int argc, char * argv[]) try
{
    using namespace rs2;

    window app(1280, 720, "RealSense Pointcloud Example");
    state app_state = { 0, 0, 0, 0, false, 0, 0 };

    glfwSetWindowUserPointer(app, &app_state);
    register_glfw_callbacks(app);

    pipeline pipe;

    pointcloud pc = rs2::context().create_pointcloud();

    pipe.start();

    texture mapped_tex;

    GLint texture_border_mode = 0x812F; // GL_CLAMP_TO_EDGE
    float tex_border_color[] = { 0.8f, 0.8f, 0.8f, 0.8f };
    rs2::points points;

    while (app)
    {
        glPopMatrix();
        glPushAttrib(GL_ALL_ATTRIB_BITS);

        float width = app.width(), height = app.height();
        glClearColor(52.0f / 255, 72.f / 255, 94.0f / 255, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        gluPerspective(60, width / height, 0.01f, 10.0f);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        gluLookAt(0, 0, 0, 0, 0, 1, 0, -1, 0);

        glTranslatef(0, 0, +0.5f + app_state.offset_y*0.05f);
        glRotated(app_state.pitch, 1, 0, 0);
        glRotated(app_state.yaw, 0, 1, 0);
        glTranslatef(0, 0, -0.5f);

        glPointSize(width / 640);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, mapped_tex.get_gl_handle());
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_border_mode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_border_mode);
        glBegin(GL_POINTS);

        auto frames = pipe.wait_for_frames();
        if (frames.size())
        {
//            std::cout << "aha!" << std::endl;
            if (auto color = frames.get_color_frame())
            {
//                std::cout << "color" << std::endl;
                mapped_tex.upload(color);
                pc.map_to(color);
            }
            if (auto depth = frames.get_depth_frame())
            {
//                std::cout << "depth" << std::endl;
                points = pc.calculate(depth);
            }

            auto vertices = points.get_vertices();
            auto tex_coords = points.get_texture_coordinates();

            for (int i = 0; i < points.size(); i++)
            {
                if (vertices[i].z)
                {
                    glVertex3fv(vertices[i]);
                    glTexCoord2fv(tex_coords[i]);
                }
            }
        }
        else
        {
            //std::cout << "oops" << std::endl;
        }

        glEnd();
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glPopAttrib();
        glPushMatrix();
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)

{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
}

void register_glfw_callbacks(window& app)
{
    glfwSetMouseButtonCallback(app, [](GLFWwindow * win, int button, int action, int mods)
    {
        auto s = (state *)glfwGetWindowUserPointer(win);
        if (button == GLFW_MOUSE_BUTTON_LEFT) s->ml = action == GLFW_PRESS;
    });

    glfwSetScrollCallback(app, [](GLFWwindow * win, double xoffset, double yoffset)
    {
        auto s = (state *)glfwGetWindowUserPointer(win);
        s->offset_x += static_cast<float>(xoffset);
        s->offset_y += static_cast<float>(yoffset);
    });

    glfwSetCursorPosCallback(app, [](GLFWwindow * win, double x, double y)
    {
        auto s = (state *)glfwGetWindowUserPointer(win);
        if (s->ml)
        {
            s->yaw -= (x - s->last_x);
            s->yaw = std::max(s->yaw, -120.0);
            s->yaw = std::min(s->yaw, +120.0);
            s->pitch += (y - s->last_y);
            s->pitch = std::max(s->pitch, -80.0);
            s->pitch = std::min(s->pitch, +80.0);
        }
        s->last_x = x;
        s->last_y = y;
    });

    glfwSetKeyCallback(app, [](GLFWwindow * win, int key, int scancode, int action, int mods)
    {
        auto s = (state *)glfwGetWindowUserPointer(win);

        bool bext = false, bint = false, bloc = false;
        if (0 == action) //on key release
        {
            if (key == GLFW_KEY_SPACE)
            {
                s->yaw = s->pitch = 0; s->offset_x = s->offset_y = 0.0;
            }
        }
    });
}