// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>

#include <sstream>
#include <iostream>
#include <memory>

using namespace rs2;
using namespace std;

struct rect
{
    float x, y;
    float w, h;
};

class texture_buffer
{
    GLuint texture;

public:

    void upload(const rs2::frame& frame)
    {
        // If the frame timestamp has changed since the last time show(...) was called, re-upload the texture
        if (!texture)
            glGenTextures(1, &texture);

        int width = 0;
        int height = 0;
        int stride = 0;
        auto format = frame.get_profile().format();
        auto data = frame.get_data();

        auto image = frame.as<video_frame>();

        if (image)
        {
            width = image.get_width();
            height = image.get_height();
            stride = image.get_stride_in_bytes();
        }

        glBindTexture(GL_TEXTURE_2D, texture);
        stride = stride == 0 ? width : stride;

        switch (format)
        {
        case RS2_FORMAT_ANY:
            throw std::runtime_error("not a valid format");

        case RS2_FORMAT_Z16:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
            break;

        case RS2_FORMAT_RGB8: case RS2_FORMAT_BGR8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        default:
            throw std::runtime_error("The requested format is not suported for rendering");
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

    }


    void draw_texture(const rect& s, const rect& t) const
    {
        glBegin(GL_QUAD_STRIP);
        {
            glTexCoord2f(s.x, s.y + s.h); glVertex2f(t.x, t.y + t.h);
            glTexCoord2f(s.x, s.y); glVertex2f(t.x, t.y);
            glTexCoord2f(s.x + s.w, s.y + s.h); glVertex2f(t.x + t.w, t.y + t.h);
            glTexCoord2f(s.x + s.w, s.y); glVertex2f(t.x + t.w, t.y);
        }
        glEnd();
    }

    void show(const rect& r, float alpha, const rect& normalized_zoom = rect{0, 0, 1, 1}) const
    {
        glEnable(GL_BLEND);

        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1 - alpha);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_TEXTURE_2D);
        draw_texture(normalized_zoom, r);

        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDisable(GL_BLEND);
    }
};

int main(int argc, char * argv[]) try
{
    rs2::pipeline p;
    p.start();

    //colorizer depth_colorizer;

    auto finished = false;
    GLFWwindow* win;
    //    log_to_file(RS2_LOG_SEVERITY_DEBUG);


    size_t max_frames = 0;
    map<int, texture_buffer> buffers;

    // Open a GLFW window
    glfwInit();
    ostringstream ss;
    ss << "CPP Capture Example ("/* << dev.get_info(RS2_CAMERA_INFO_NAME) <<*/ ")";

    win = glfwCreateWindow(1280, 720, ss.str().c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(win);

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();

        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        frameset frames;

        frames = p.wait_for_frames();

        //auto d = frames.get<depth_frame>();
        // d.get_distance(10,10);
        //auto colored_depth = depth_colorizer(d);

        // auto color_frame = frames.get<rgb_frame>();

        //render(colored_depth);
        //render(color_frame);


        if(frames.size() == 0)
            continue;


        for (auto&& frame : frames)
        {
            buffers[frame.get_profile().stream_type()].upload(frame);
        }

        auto tiles_horisontal = static_cast<int>(ceil(sqrt(buffers.size())));
        auto tiles_vertical = ceil(static_cast<float>(buffers.size()) / tiles_horisontal);
        auto tile_w = static_cast<float>(static_cast<float>(w) / tiles_horisontal);
        auto tile_h = static_cast<float>(static_cast<float>(h) / tiles_vertical);

        // Clear the framebuffer
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the images
        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);

        for (auto buffer = buffers.begin(); buffer != buffers.end(); ++buffer)
        {
            auto index = std::distance(buffers.begin(), buffer);
            auto col_id = index / tiles_horisontal;
            auto row_id = index % tiles_horisontal;

            buffer->second.show({ row_id * tile_w, static_cast<float>(col_id * tile_h), tile_w, tile_h }, 1);
        }
        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch (const error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
}
catch (const exception & e)
{
    cerr << e.what() << endl;
}



