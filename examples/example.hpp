// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

//////////////////////////////
// Basic Data Types         //
//////////////////////////////

struct float3 { float x, y, z; };
struct float2 { float x, y; };

struct rect
{
    float x, y;
    float w, h;

    // Create new rect within original boundaries with give aspect ration
    rect adjust_ratio(float2 size) const
    {
        auto H = static_cast<float>(h), W = static_cast<float>(h) * size.x / size.y;
        if (W > w)
        {
            auto scale = w / W;
            W *= scale;
            H *= scale;
        }

        return{ x + (w - W) / 2, y + (h - H) / 2, W, H };
    }
};

//////////////////////////////
// Simple font loading code //
//////////////////////////////

#include "../third-party/stb_easy_font.h"

inline void draw_text(int x, int y, const char * text)
{
    char buffer[60000]; // ~300 chars
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print((float)x, (float)(y - 7), (char *)text, nullptr, buffer, sizeof(buffer)));
    glDisableClientState(GL_VERTEX_ARRAY);
}

////////////////////////
// Image display code //
////////////////////////
class texture
{
public:
    void render(const rs2::frameset& frames, int window_width, int window_height)
    {
        std::vector<rs2::video_frame> supported_frames;
        for (auto f : frames)
        {
            if (can_render(f))
                supported_frames.push_back(f);
        }
        if (supported_frames.empty())
            return;

        std::sort(supported_frames.begin(), supported_frames.end(), [](rs2::frame first, rs2::frame second)
            { return first.get_profile().stream_type() < second.get_profile().stream_type();  });

        auto image_grid = calc_grid(float2{ (float)window_width, (float)window_height }, supported_frames);

        int image_index = 0;
        for (auto f : supported_frames)
        {
            upload(f);
            show(image_grid.at(image_index));
            image_index++;
        }
    }

    void render(const rs2::video_frame& frame, const rect& r)
    {
        upload(frame);
        show(r.adjust_ratio({ float(width), float(height) }));
    }

    void upload(const rs2::video_frame& frame)
    {
        if (!frame) return;

        if (!gl_handle)
            glGenTextures(1, &gl_handle);
        GLenum err = glGetError();

        auto format = frame.get_profile().format();
        width = frame.get_width();
        height = frame.get_height();
        stream = frame.get_profile().stream_type();

        glBindTexture(GL_TEXTURE_2D, gl_handle);

        switch (format)
        {
        case RS2_FORMAT_RGB8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, frame.get_data());
            break;
        case RS2_FORMAT_RGBA8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, frame.get_data());
            break;
        case RS2_FORMAT_Y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame.get_data());
            break;
        default:
            throw std::runtime_error("The requested format is not supported by this demo!");
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint get_gl_handle() { return gl_handle; }

    void show(const rect& r) const
    {
        if (!gl_handle)
            return;

        glBindTexture(GL_TEXTURE_2D, gl_handle);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUAD_STRIP);
        glTexCoord2f(0.f, 1.f); glVertex2f(r.x, r.y + r.h);
        glTexCoord2f(0.f, 0.f); glVertex2f(r.x, r.y);
        glTexCoord2f(1.f, 1.f); glVertex2f(r.x + r.w, r.y + r.h);
        glTexCoord2f(1.f, 0.f); glVertex2f(r.x + r.w, r.y);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        draw_text((int)r.x + 15, (int)r.y + 20, rs2_stream_to_string(stream));
    }

private:
    GLuint gl_handle = 0;
    int width = 0;
    int height = 0;
    rs2_stream stream = RS2_STREAM_ANY;

    bool can_render(const rs2::frame& f) const
    {
        auto format = f.get_profile().format();
        switch (format)
        {
        case RS2_FORMAT_RGB8:
        case RS2_FORMAT_RGBA8:
        case RS2_FORMAT_Y8:
            return true;
        default:
            return false;
        }
    }

    rect calc_grid(float2 window, int streams)
    {
        if (window.x <= 0 || window.y <= 0 || streams <= 0)
            throw std::runtime_error("invalid window configuration request, failed to calculate window grid");
        float ratio = window.x / window.y;
        auto x = sqrt(ratio * (float)streams);
        auto y = (float)streams / x;
        auto w = round(x);
        auto h = round(y);
        if (w == 0 || h == 0)
            throw std::runtime_error("invalid window configuration request, failed to calculate window grid");
        while (w*h > streams)
            h > w ? h-- : w--;
        while (w*h < streams)
            h > w ? w++ : h++;
        auto new_w = round(window.x / w);
        auto new_h = round(window.y / h);
        // column count, line count, cell width cell height
        return rect{ static_cast<float>(w), static_cast<float>(h), static_cast<float>(new_w), static_cast<float>(new_h) };
    }

    std::vector<rect> calc_grid(float2 window, std::vector<rs2::video_frame>& frames)
    {
        auto grid = calc_grid(window, frames.size());

        int index = 0;
        std::vector<rect> rv;
        int curr_line = -1;
        for (auto f  : frames)
        {
            auto mod = index % (int)grid.x;
            auto fw = (float)frames[index].get_width();
            auto fh = (float)frames[index].get_height();

            float cell_x_postion = (float)(mod * grid.w);
            if (mod == 0) curr_line++;
            float cell_y_position = curr_line * grid.h;

            auto r = rect{ cell_x_postion, cell_y_position, grid.w, grid.h };
            rv.push_back(r.adjust_ratio(float2{ fw, fh }));
            index++;
        }

        return rv;
    }
};

class window
{
public:
    std::function<void(bool)>           on_left_mouse = [](bool) {};
    std::function<void(double, double)> on_mouse_scroll = [](double, double) {};
    std::function<void(double, double)> on_mouse_move = [](double, double) {};
    std::function<void(int)>            on_key_release = [](int) {};

    window(int width, int height, const char* title)
        : _width(width), _height(height)
    {
        glfwInit();
        win = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!win)
            throw std::runtime_error("Could not open OpenGL window, please check your graphic drivers or use the textual SDK tools");
        glfwMakeContextCurrent(win);

        glfwSetWindowUserPointer(win, this);
        glfwSetMouseButtonCallback(win, [](GLFWwindow * win, int button, int action, int mods)
        {
            auto s = (window*)glfwGetWindowUserPointer(win);
            if (button == 0) s->on_left_mouse(action == GLFW_PRESS);
        });

        glfwSetScrollCallback(win, [](GLFWwindow * win, double xoffset, double yoffset)
        {
            auto s = (window*)glfwGetWindowUserPointer(win);
            s->on_mouse_scroll(xoffset, yoffset);
        });

        glfwSetCursorPosCallback(win, [](GLFWwindow * win, double x, double y)
        {
            auto s = (window*)glfwGetWindowUserPointer(win);
            s->on_mouse_move(x, y);
        });

        glfwSetKeyCallback(win, [](GLFWwindow * win, int key, int scancode, int action, int mods)
        {
            auto s = (window*)glfwGetWindowUserPointer(win);
            if (0 == action) // on key release
            {
                s->on_key_release(key);
            }
        });
    }

    float width() const { return float(_width); }
    float height() const { return float(_height); }

    operator bool()
    {
        glPopMatrix();
        glfwSwapBuffers(win);

        auto res = !glfwWindowShouldClose(win);

        glfwPollEvents();
        glfwGetFramebufferSize(win, &_width, &_height);

        // Clear the framebuffer
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, _width, _height);

        // Draw the images
        glPushMatrix();
        glfwGetWindowSize(win, &_width, &_height);
        glOrtho(0, _width, _height, 0, -1, +1);

        return res;
    }

    ~window()
    {
        glfwDestroyWindow(win);
        glfwTerminate();
    }

    operator GLFWwindow*() { return win; }

private:
    GLFWwindow * win;
    int _width, _height;
};

// Struct for managing rotation of pointcloud view
struct glfw_state {
    glfw_state() : yaw(15.0), pitch(15.0), last_x(0.0), last_y(0.0),
        ml(false), offset_x(2.f), offset_y(2.f), tex() {}
    double yaw;
    double pitch;
    double last_x;
    double last_y;
    bool ml;
    float offset_x;
    float offset_y;
    texture tex;
};


// Handles all the OpenGL calls needed to display the point cloud
void draw_pointcloud(float width, float height, glfw_state& app_state, rs2::points& points)
{
    if (!points)
        return;

    // OpenGL commands that prep screen for the pointcloud
    glPopMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glClearColor(153.f / 255, 153.f / 255, 153.f / 255, 1);
    glClear(GL_DEPTH_BUFFER_BIT);

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

// Registers the state variable and callbacks to allow mouse control of the pointcloud
void register_glfw_callbacks(window& app, glfw_state& app_state)
{
    app.on_left_mouse = [&](bool pressed)
    {
        app_state.ml = pressed;
    };

    app.on_mouse_scroll = [&](double xoffset, double yoffset)
    {
        app_state.offset_x -= static_cast<float>(xoffset);
        app_state.offset_y -= static_cast<float>(yoffset);
    };

    app.on_mouse_move = [&](double x, double y)
    {
        if (app_state.ml)
        {
            app_state.yaw -= (x - app_state.last_x);
            app_state.yaw = std::max(app_state.yaw, -120.0);
            app_state.yaw = std::min(app_state.yaw, +120.0);
            app_state.pitch += (y - app_state.last_y);
            app_state.pitch = std::max(app_state.pitch, -80.0);
            app_state.pitch = std::min(app_state.pitch, +80.0);
        }
        app_state.last_x = x;
        app_state.last_y = y;
    };

    app.on_key_release = [&](int key)
    {
        if (key == 32) // Escape
        {
            app_state.yaw = app_state.pitch = 0; app_state.offset_x = app_state.offset_y = 0.0;
        }
    };
}
