// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

#include <algorithm>            // std::min, std::max

// Struct for managing rotation of pointcloud view
struct state { 
    state() : yaw(0.0), pitch(0.0), last_x(0.0), last_y(0.0),
        ml(false), offset_x(0.0f), offset_y(0.0f), tex() {}
    double yaw, pitch, last_x, last_y; bool ml; float offset_x, offset_y; texture tex; 
};

// Helper functions
void register_glfw_callbacks(window& app, state& app_state);
void draw_pointcloud(window& app, state& app_state, rs2::points& points);

int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Pointcloud Example");
    // Construct an object to manage view state
    state app_state;
    // register callbacks to allow manipulation of the pointcloud
    register_glfw_callbacks(app, app_state);

    using namespace rs2;
    // Declare pointcloud object, for calculating pointclouds and texture mappings
    pointcloud pc = rs2::context().create_pointcloud();
    // We want the points object to be persistent so we can display the last cloud when a frame drops
    rs2::points points;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    while (app) // Application still alive?
    {
        // Wait for the next set of frames from the camera
        auto frames = pipe.wait_for_frames();
        if (auto color = frames.get_color_frame())
        {
            // Tell pointcloud object to map to this color frame
            pc.map_to(color);

            // Upload the color frame to OpenGL
            app_state.tex.upload(color);
        }
        if (auto depth = frames.get_depth_frame())
        {
            // If we got a depth frame, generate the pointcloud and texture mappings
            points = pc.calculate(depth);
        }

        // Draw the pointcloud
        draw_pointcloud(app, app_state, points);
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

// Registers the state variable and callbacks with glfw to allow mouse control of the pointcloud
void register_glfw_callbacks(window& app, state& app_state)
{
    glfwSetWindowUserPointer(app, &app_state);
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

// Handles all the OpenGL calls needed to display the point cloud
void draw_pointcloud(window& app, state& app_state, rs2::points& points)
{
    // OpenGL commands that prep screen for the pointcloud
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
    glBindTexture(GL_TEXTURE_2D, app_state.tex.get_gl_handle());
    float tex_border_color[] = { 0.8f, 0.8f, 0.8f, 0.8f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); // GL_CLAMP_TO_EDGE
    glBegin(GL_POINTS);


    /* this segment actually prints the pointcloud */
    auto vertices = points.get_vertices();              // get vertices
    auto tex_coords = points.get_texture_coordinates(); // and texture coordinates
    for (int i = 0; i < points.size(); i++)
    {
        if (vertices[i].z)
        {
            // upload the point and texture coordinates only for points we have depth data for
            glVertex3fv(vertices[i]);
            glTexCoord2fv(tex_coords[i]);
        }
    }

    // OpenGL cleanup
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glPushMatrix();
}
