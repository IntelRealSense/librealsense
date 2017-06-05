// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"
#include <imgui.h>
#include "imgui_impl_glfw.h"

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

struct texture_stream
{
    rs2_stream stream;
    rs2_format format;
    std::string title;
};

int main(int argc, char * argv[])
{
    log_to_console(RS2_LOG_SEVERITY_WARN);
    //log_to_file(log_severity::debug, "librealsense.log");

    auto finished = false;
    GLFWwindow* win;

    context ctx;

    util::device_hub devs(ctx);

    while (!finished)
    {
        try{
            auto dev = devs.wait_for_device();


            // Configure streams to run at 30 frames per second
            util::config config;
            config.enable_stream(RS2_STREAM_DEPTH, preset::best_quality);

            std::vector<texture_stream> available_streams;
            std::vector<const char*> stream_names;
            int selected_stream = 0;

            // try to open color stream, but fall back to IR if the camera doesn't support it
            rs2_stream mapped;
            if (config.can_enable_stream(dev, RS2_STREAM_FISHEYE, 0, 0, 0, RS2_FORMAT_RAW8))
            {
                available_streams.push_back({ RS2_STREAM_FISHEYE, RS2_FORMAT_RAW8, "Fish-Eye" });
            }
            if (config.can_enable_stream(dev, RS2_STREAM_COLOR, 0, 0, 0, RS2_FORMAT_RGB8))
            {
                available_streams.push_back({ RS2_STREAM_COLOR, RS2_FORMAT_RGB8, "Color" });
            }
            if (config.can_enable_stream(dev, RS2_STREAM_INFRARED, 0, 0, 0, RS2_FORMAT_RGB8))
            {
                available_streams.push_back({ RS2_STREAM_INFRARED, RS2_FORMAT_RGB8, "Artifical Color" });
            }
            if (config.can_enable_stream(dev, RS2_STREAM_INFRARED, 0, 0, 0, RS2_FORMAT_Y8))
            {
                available_streams.push_back({ RS2_STREAM_INFRARED, RS2_FORMAT_Y8, "Infrared" });
            }
            if (config.can_enable_stream(dev, RS2_STREAM_INFRARED2, 0, 0, 0, RS2_FORMAT_Y8))
            {
                available_streams.push_back({ RS2_STREAM_INFRARED2, RS2_FORMAT_Y8, "Infrared2" });
            }

            if (available_streams.size() == 0)
            {
                throw runtime_error("Couldn't configure camera for demo");
            }

            for (auto&& s : available_streams) stream_names.push_back(s.title.c_str());

            auto&& selected = available_streams[selected_stream];
            config.enable_stream(selected.stream, 0, 0, 0, selected.format);
            mapped = selected.stream;
            auto stream = config.open(dev);

            state app_state = {0, 0, 0, 0, false, &dev};

            glfwInit();
            ostringstream ss; ss << "CPP Point Cloud Example (" << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME) << ")";
            win = glfwCreateWindow(1280, 720, ss.str().c_str(), 0, 0);
            ImGui_ImplGlfw_Init(win, true);
            glfwMakeContextCurrent(win);


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

            auto syncer = dev.create_syncer();
            stream.start(syncer);

            texture_buffer mapped_tex;
            const uint16_t * depth;

            rs2_extrinsics extrin = stream.get_extrinsics(RS2_STREAM_DEPTH, mapped);
            rs2_intrinsics mapped_intrin = stream.get_intrinsics(mapped);

            const rs2_intrinsics depth_intrin = stream.get_intrinsics(RS2_STREAM_DEPTH);

            while (devs.is_connected(dev) && !glfwWindowShouldClose(win))
            {


                glfwPollEvents();
                ImGui_ImplGlfw_NewFrame();

                auto frames = syncer.wait_for_frames(500);
                if (frames.size() == 0)
                    continue;

                glPushAttrib(GL_ALL_ATTRIB_BITS);

                for (auto&& frame : frames)
                {
                    if (frame.get_stream_type() == RS2_STREAM_DEPTH)
                    {
                        depth = reinterpret_cast<const uint16_t *>(frame.get_data());
                    }

                    if (frame.get_stream_type() == mapped)
                    {
                        mapped_tex.upload(frame);
                    }
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

                // Determine depth value corresponding to one meter
                auto depth_units = 1.f;
                if (dev.supports(RS2_OPTION_DEPTH_UNITS))
                    depth_units = dev.get_option(RS2_OPTION_DEPTH_UNITS);

                vector<uint8_t> image;
                auto points = depth_to_points(image, depth_intrin, depth, depth_units);

                for (int y=0; y<depth_intrin.height; ++y)
                {
                    for(int x=0; x<depth_intrin.width; ++x)
                    {
                        if(points->z)
                        {
                            auto trans = transform(&extrin, *points);
                            auto tex_xy = project_to_texcoord(&mapped_intrin, trans);

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

                // Draw GUI:
                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
                ImGui::SetNextWindowPos({ 0, 0 });
                ImGui::SetNextWindowSize({ 200.f, (float)height });
                ImGui::Begin("Stream Selector", nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

                rs2_error* e = nullptr;
                ImGui::Text("VERSION: %s", api_version_to_string(rs2_get_api_version(&e)).c_str());


                ImGui::Text("Texture Source:");
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo(" texture", &selected_stream, stream_names.data(), stream_names.size()))
                {
                    stream.stop();
                    dev.close();
                    auto&& selected = available_streams[selected_stream];
                    util::config config;
                    config.enable_stream(RS2_STREAM_DEPTH, preset::best_quality);
                    config.enable_stream(selected.stream, 0, 0, 0, selected.format);
                    mapped = selected.stream;
                    stream = config.open(dev);
                    stream.start(syncer);
                    extrin = stream.get_extrinsics(RS2_STREAM_DEPTH, mapped);
                    mapped_intrin = stream.get_intrinsics(mapped);
                }
                ImGui::PopItemWidth();
                ImGui::Text("(Left Mouse Drag):\nRotate Viewport");
                ImGui::End();
                ImGui::PopStyleColor();

                ImGui::Render();
                glfwSwapBuffers(win);
            }
            if (glfwWindowShouldClose(win))
                finished = true;
        }
        catch(const error & e)

        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
        }
        catch(const exception & e)
        {
            cerr << e.what() << endl;
        }

        glfwDestroyWindow(win);
        ImGui_ImplGlfw_Shutdown();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
