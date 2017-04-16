// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"

#include <chrono>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

using namespace rs2;
using namespace std;

inline void glVertex(const float3 & vertex) { glVertex3fv(&vertex.x); }
inline void glTexCoord(const float2 & tex_coord) { glTexCoord2fv(&tex_coord.x); }

struct state { double yaw, pitch, lastX, lastY; bool ml; device * dev; };

template<class MAP_DEPTH> void deproject_depth(float * points, const rs2_intrinsics & intrin, const uint16_t * depth, MAP_DEPTH map_depth)
{
    for (int y = 0; y<intrin.height; ++y)
    {
        for (int x = 0; x<intrin.width; ++x)
        {
            const float pixel[] = { (float)x, (float)y };
            rs2_deproject_pixel_to_point(points, &intrin, pixel, map_depth(*depth++));
            points += 3;
        }
    }
}

const float3 * depth_to_points(vector<uint8_t> &image, const rs2_intrinsics &depth_intrinsics, const uint16_t * depth_image, float depth_scale)
{
    image.resize(depth_intrinsics.width * depth_intrinsics.height * 12);

    deproject_depth(reinterpret_cast<float *>(image.data()), depth_intrinsics, depth_image, [depth_scale](uint16_t z) { return depth_scale * z; });

    return reinterpret_cast<float3 *>(image.data());
}

float3 transform(const rs2_extrinsics *extrin, const float3 &point) { float3 p = {}; rs2_transform_point_to_point(&p.x, extrin, &point.x); return p; }
float2 project(const rs2_intrinsics *intrin, const float3 & point) { float2 pixel = {}; rs2_project_point_to_pixel(&pixel.x, intrin, &point.x); return pixel; }
float2 pixel_to_texcoord(const rs2_intrinsics *intrin, const float2 & pixel) { return{ (pixel.x + 0.5f) / intrin->width, (pixel.y + 0.5f) / intrin->height }; }
float2 project_to_texcoord(const rs2_intrinsics *intrin, const float3 & point) { return pixel_to_texcoord(intrin, project(intrin, point)); }



int main(int argc, char * argv[]) try
{
    log_to_console(RS2_LOG_SEVERITY_WARN);
    //log_to_file(log_severity::debug, "librealsense.log");

    context ctx;
    auto list = ctx.query_devices();
    if (list.size() == 0)
        throw runtime_error("No device detected. Is it plugged in?");

    auto dev = list[0];

    // Configure streams to run at 30 frames per second
    util::config config;
    config.enable_stream(RS2_STREAM_DEPTH, preset::best_quality);

    // try to open color stream, but fall back to IR if the camera doesn't support it
    rs2_stream mapped;
    if (config.can_enable_stream(dev, RS2_STREAM_FISHEYE, 640, 480, 60, RS2_FORMAT_RAW8))
    {
        mapped = RS2_STREAM_FISHEYE;
        config.enable_stream(RS2_STREAM_FISHEYE, 640, 480,  60, RS2_FORMAT_RAW8);
    }
    else if (config.can_enable_stream(dev, RS2_STREAM_COLOR, preset::best_quality)) {
        mapped = RS2_STREAM_COLOR;
        config.enable_stream(RS2_STREAM_COLOR, preset::best_quality);
    }
    else if (config.can_enable_stream(dev, RS2_STREAM_INFRARED, 0, 0, 0, RS2_FORMAT_RGB8)) {
        mapped = RS2_STREAM_INFRARED;
        config.enable_stream(RS2_STREAM_INFRARED, 0, 0, 0, RS2_FORMAT_RGB8);
    }
    else if (config.can_enable_stream(dev, RS2_STREAM_INFRARED, preset::best_quality)) {
        mapped = RS2_STREAM_INFRARED;
        config.enable_stream(RS2_STREAM_INFRARED, preset::best_quality);
    }
    else if (config.can_enable_stream(dev, RS2_STREAM_INFRARED2, preset::best_quality)) {
        mapped = RS2_STREAM_INFRARED2;
        config.enable_stream(RS2_STREAM_INFRARED2, preset::best_quality);
    }
    else {
        throw runtime_error("Couldn't configure camera for demo");
    }

    auto stream = config.open(dev);

    state app_state = {0, 0, 0, 0, false, &dev};

    glfwInit();
    ostringstream ss; ss << "CPP Point Cloud Example (" << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME) << ")";
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
            s->yaw = max(s->yaw, -120.0);
            s->yaw = min(s->yaw, +120.0);
            s->pitch += (y - s->lastY);
            s->pitch = max(s->pitch, -80.0);
            s->pitch = min(s->pitch, +80.0);
        }
        s->lastX = x;
        s->lastY = y;
    });

    util::syncer syncer;
    stream.start(syncer);

    glfwMakeContextCurrent(win);
    texture_buffer mapped_tex;
    const uint16_t * depth;

    const rs2_extrinsics extrin = stream.get_extrinsics(RS2_STREAM_DEPTH, mapped);
    const rs2_intrinsics depth_intrin = stream.get_intrinsics(RS2_STREAM_DEPTH);
    const rs2_intrinsics mapped_intrin = stream.get_intrinsics(mapped);

    vector<uint8_t> undistorted_fisheye(640*480, 0);

    int frame_count = 0; float time = 0, fps = 0;
    auto t0 = chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();
        auto frames = syncer.wait_for_frames();

        if (frames.size() == 0)
            continue;

        auto t1 = chrono::high_resolution_clock::now();
        time += chrono::duration<float>(t1-t0).count();
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
            if (frame.get_stream_type() == mapped)
            {           
                mapped_tex.upload(frame);
            }

            if (frame.get_stream_type() == RS2_STREAM_DEPTH)
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
        glBindTexture(GL_TEXTURE_2D, mapped_tex.get_gl_handle());
        glBegin(GL_POINTS);

        auto max_depth = *max_element(depth,depth+640*480);

        vector<uint8_t> image;
        auto points = depth_to_points(image, depth_intrin, depth, dev.get_depth_scale());
        //auto depth = reinterpret_cast<const uint16_t *>(dev.get_frame_data(stream::depth));

        for (int y=0; y<depth_intrin.height; ++y)
        {
            for(int x=0; x<depth_intrin.width; ++x)
            {
                if(points->z)
                {
                    auto trans = transform(&extrin, *points);
                    auto tex_xy = project_to_texcoord(&mapped_intrin, trans);

                    //Its seems that at the moment fisheye extrinsics are roteted in x and y in 180 degrees
                    //as a temporery work around we rotete the coordinates of the texture
//                    if(mapped == RS2_STREAM_FISHEYE)
//                    {
//                        tex_xy.x = 1 - tex_xy.x;
//                        tex_xy.y = 1 - tex_xy.y;
//                    }

                    glTexCoord(tex_xy);
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

        ostringstream ss; ss << fps << " FPS";
        draw_text(20, 40, ss.str().c_str());
        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch(const error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
catch(const exception & e)
{
    cerr << e.what() << endl;
    return EXIT_FAILURE;
}
