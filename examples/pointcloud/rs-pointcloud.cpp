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
#include <algorithm>

#ifdef _MSC_VER
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER  0x812D
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE    0x812F
#endif
#endif

using namespace rs2;
using namespace std;

inline void glVertex(const float3 & vertex) { glVertex3fv(&vertex.x); }
inline void glTexCoord(const float2 & tex_coord) { glTexCoord2fv(&tex_coord.x); }

struct state { double yaw, pitch, last_x, last_y; bool ml; float offset_x, offset_y; device * dev; };

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
    auto hw_reset_enable = true;
    auto rgb_flipflop_enable = false;
    rs2_extrinsics extrin{};
    rs2_intrinsics mapped_intrin{};


    context ctx;

    util::device_hub devs(ctx);

    while (!finished)
    {
        GLFWwindow* win = nullptr;
        try{
            auto dev = devs.wait_for_device();
            auto depth_camera = dev.first<depth_sensor>();
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

            state app_state = {0, 0, 0, 0, false, 0, 0, &dev};

            glfwInit();
            ostringstream ss; ss << "RealSense Point Cloud Example (" << dev.get_info(RS2_CAMERA_INFO_NAME) << ")";
            win = glfwCreateWindow(1280, 720, ss.str().c_str(), 0, 0);
            ImGui_ImplGlfw_Init(win, true);
            glfwMakeContextCurrent(win);

            glfwSetWindowUserPointer(win, &app_state);
            glfwSetMouseButtonCallback(win, [](GLFWwindow * win, int button, int action, int mods)
            {
                auto s = (state *)glfwGetWindowUserPointer(win);
                if(button == GLFW_MOUSE_BUTTON_LEFT) s->ml = action == GLFW_PRESS;
            });

            glfwSetScrollCallback(win, [](GLFWwindow * win, double xoffset, double yoffset)
            {
                auto s = (state *)glfwGetWindowUserPointer(win);
                s->offset_x += static_cast<float>(xoffset);
                s->offset_y += static_cast<float>(yoffset);
            });

            glfwSetCursorPosCallback(win, [](GLFWwindow * win, double x, double y)
            {
                auto s = (state *)glfwGetWindowUserPointer(win);
                if(s->ml)
                {
                    s->yaw -= (x - s->last_x);
                    s->yaw = max(s->yaw, -120.0);
                    s->yaw = min(s->yaw, +120.0);
                    s->pitch += (y - s->last_y);
                    s->pitch = max(s->pitch, -80.0);
                    s->pitch = min(s->pitch, +80.0);
                }
                s->last_x = x;
                s->last_y = y;
            });

            glfwSetKeyCallback(win, [](GLFWwindow * win, int key, int scancode, int action, int mods)
            {
                auto s = (state *)glfwGetWindowUserPointer(win);

                bool bext = false, bint = false, bloc= false;
                if (0 == action) //on key release
                {
                    if (key == GLFW_KEY_SPACE)
                    {
                        s->yaw = s->pitch = 0; s->offset_x = s->offset_y = 0.0;
                    }
                }
            });

            syncer syncer;
            stream.start(syncer);

            texture_buffer mapped_tex;
            const uint16_t * depth;

            extrin = stream.get_extrinsics(RS2_STREAM_DEPTH, mapped);
            mapped_intrin = stream.get_intrinsics(mapped);

            const rs2_intrinsics depth_intrin = stream.get_intrinsics(RS2_STREAM_DEPTH);

            bool rgb_rotation_btn = val_in_range(std::string(dev.get_info(RS2_CAMERA_INFO_NAME)),
                { std::string("0AD3") ,std::string("0B07") });
            bool texture_wrapping_on = true;
            GLint texture_border_mode = GL_CLAMP_TO_EDGE; // GL_CLAMP_TO_BORDER
            float tex_border_color[] = { 0.8f, 0.8f, 0.8f, 0.8f };

            while (devs.is_connected(dev) && !glfwWindowShouldClose(win))
            {
                glfwPollEvents();
                ImGui_ImplGlfw_NewFrame();

                auto frames = syncer.wait_for_frames(500);
                if (frames.size() == 0)
                    continue;

                bool has_depth = false;
                for (auto&& f : frames)
                    if (f.get_stream_type() == RS2_STREAM_DEPTH)
                        has_depth = true;
                if (!has_depth) continue;

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
                gluPerspective(60, (float)width/height, 0.01f, 10.0f);

                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                gluLookAt(0,0,0, 0,0,1, 0,-1,0);

                glTranslatef(0,0,+0.5f+ app_state.offset_y*0.05f);
                glRotated(app_state.pitch, 1, 0, 0);
                glRotated(app_state.yaw, 0, 1, 0);
                glTranslatef(0,0,-0.5f);

                glPointSize((float)width/640);
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, mapped_tex.get_gl_handle());
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_border_mode);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_border_mode);
                glBegin(GL_POINTS);

                // Determine depth value corresponding to one meter
                auto depth_units = depth_camera.get_depth_scale();

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
                if (ImGui::Combo(" texture", &selected_stream, stream_names.data(), static_cast<int>(stream_names.size())))
                {
                    stream.stop();
                    stream.close();
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
                ImGui::Text("\nApplication Controls:");
                ImGui::Text("Mouse Left: Rotate Viewport");
                ImGui::Text("Mouse Scroll: Zoom in/out");
                ImGui::Text("Space Key: Reset View");
                if (ImGui::Checkbox("Texture wrapping", &texture_wrapping_on))
                    texture_border_mode = texture_wrapping_on ? GL_CLAMP_TO_EDGE : GL_CLAMP_TO_BORDER;
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Control texture mapping outside of coverage area");
                if ((rgb_rotation_btn) && ImGui::Button("Adjust RGB orientation", ImVec2(160, 20)))
                {
                    rotate_rgb_image(dev, mapped_intrin.width);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Rotate RGB Sensor 180 deg");
                }

                if (ImGui::ButtonEx("Reset Device", { 160, 20 }, hw_reset_enable ? 0 : ImGuiButtonFlags_Disabled))
                {
                    try
                    {
                        dev.hardware_reset();
                    }
                    catch (...)
                    {
                    }
                }

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

        if(win)
        {
            glfwDestroyWindow(win);
        }
        ImGui_ImplGlfw_Shutdown();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
