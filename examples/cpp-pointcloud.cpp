// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include "example.hpp"

#include <chrono>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

inline void glVertex(const float3 & vertex) { glVertex3fv(&vertex.x); }
inline void glTexCoord(const float2 & tex_coord) { glTexCoord2fv(&tex_coord.x); }

struct state { double yaw, pitch, lastX, lastY; bool ml; rs::device * dev; };

template<class MAP_DEPTH> void deproject_depth(float * points, const rs_intrinsics & intrin, const uint16_t * depth, MAP_DEPTH map_depth)
{
    for (int y = 0; y<intrin.height; ++y)
    {
        for (int x = 0; x<intrin.width; ++x)
        {
            const float pixel[] = { (float)x, (float)y };
            rs_deproject_pixel_to_point(points, &intrin, pixel, map_depth(*depth++));
            points += 3;
        }
    }
}

const float3 * depth_to_points(std::vector<uint8_t> &image, const rs_intrinsics &depth_intrinsics, const uint16_t * depth_image, float depth_scale)
{
    image.resize(depth_intrinsics.width * depth_intrinsics.height * 12);

    deproject_depth(reinterpret_cast<float *>(image.data()), depth_intrinsics, depth_image, [depth_scale](uint16_t z) { return depth_scale * z; });

    return reinterpret_cast<float3 *>(image.data());
}

float3 transform(const rs_extrinsics *extrin, const float3 &point) { float3 p = {}; rs_transform_point_to_point(&p.x, extrin, &point.x); return p; }
float2 project(const rs_intrinsics *intrin, const float3 & point) { float2 pixel = {}; rs_project_point_to_pixel(&pixel.x, intrin, &point.x); return pixel; }
float2 pixel_to_texcoord(const rs_intrinsics *intrin, const float2 & pixel) { return{ (pixel.x + 0.5f) / intrin->width, (pixel.y + 0.5f) / intrin->height }; }
float2 project_to_texcoord(const rs_intrinsics *intrin, const float3 & point) { return pixel_to_texcoord(intrin, project(intrin, point)); }

int main(int argc, char * argv[]) try
{
    rs::log_to_console(RS_LOG_SEVERITY_WARN);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    auto list = ctx.query_devices();
    if (list.size() == 0)
        throw std::runtime_error("No device detected. Is it plugged in?");

    auto dev = list[0];

    // Configure all supported streams to run at 30 frames per second
    rs::util::config config;
    config.enable_stream(RS_STREAM_INFRARED, rs::preset::best_quality);
    config.enable_stream(RS_STREAM_DEPTH, rs::preset::best_quality);
    auto stream = config.open(dev);
    
    state app_state = {0, 0, 0, 0, false, &dev};
    
    glfwInit();
    std::ostringstream ss; ss << "CPP Point Cloud Example (" << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME) << ")";
    GLFWwindow * win = glfwCreateWindow(640, 480, ss.str().c_str(), 0, 0);
    glfwSetWindowUserPointer(win, &app_state);
        
    glfwSetMouseButtonCallback(win, [](GLFWwindow * win, int button, int action, int /*mods*/)
    {
        auto s = (state *)glfwGetWindowUserPointer(win);
        if(button == GLFW_MOUSE_BUTTON_LEFT) s->ml = action == GLFW_PRESS;
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

    rs::util::syncer syncer;
    stream.start(syncer);

    glfwMakeContextCurrent(win);
    texture_buffer color_tex;
    const uint16_t * depth;
    
    const rs_extrinsics extrin = stream.get_extrinsics(RS_STREAM_DEPTH, RS_STREAM_DEPTH);
    const rs_intrinsics depth_intrin = stream.get_intrinsics(RS_STREAM_DEPTH);
    const rs_intrinsics color_intrin = stream.get_intrinsics(RS_STREAM_INFRARED);

    int frame_count = 0; float time = 0, fps = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        auto frames = syncer.wait_for_frames();

        if (frames.size() == 0)
            continue;

        auto t1 = std::chrono::high_resolution_clock::now();
        time += std::chrono::duration<float>(t1-t0).count();
        t0 = t1;
        ++frame_count;
        if(time > 0.5f)
        {
            fps = frame_count / time;
            frame_count = 0;
            time = 0;
        }
     
        glPushAttrib(GL_ALL_ATTRIB_BITS);

        for (auto&& frame : frames) {
            if (frame.get_stream_type() == RS_STREAM_INFRARED)
                color_tex.upload(frame);
            if (frame.get_stream_type() == RS_STREAM_DEPTH)
                depth = reinterpret_cast<const uint16_t *>(frame.get_data());
        }

        int width, height;
        glfwGetFramebufferSize(win, &width, &height);
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
        glBindTexture(GL_TEXTURE_2D, color_tex.get_gl_handle());
        glBegin(GL_POINTS);

        auto max_depth = *std::max_element(depth,depth+640*480);
        
        std::vector<uint8_t> image;
        auto points = depth_to_points(image, depth_intrin, depth, dev.get_depth_scale());
        //auto depth = reinterpret_cast<const uint16_t *>(dev.get_frame_data(rs::stream::depth));
        
        for (int y=0; y<depth_intrin.height; ++y)
        {
            for(int x=0; x<depth_intrin.width; ++x)
            {
                if(points->z) //if(uint16_t d = *depth++)
                {
                    //const rs::float3 point = depth_intrin.deproject({static_cast<float>(x),static_cast<float>(y)}, d*depth_scale);
                    glTexCoord(project_to_texcoord(&color_intrin, transform(&extrin, *points)));
                    glVertex(*points);
                }
                ++points;
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
        
        std::ostringstream ss; ss << fps << " FPS";
        draw_text(20, 40, ss.str().c_str());
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
