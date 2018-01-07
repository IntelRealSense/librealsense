// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <algorithm>            // std::min, std::max

void render_slider_int(rect location, char* name, char* label, int* slider_param,
                       int min, int max, ImGuiWindowFlags flags, char* description);
void render_slider_float(rect location, char* name, char* label, float* slider_param,
                         float min, float max, ImGuiWindowFlags flags, char* description);
void render_checkbox(rect location, bool* checkbox_param, ImGuiWindowFlags flags, char* label);

/*
// Struct for managing rotation of pointcloud view
struct state {
    state() : yaw(0.0), pitch(0.0), last_x(0.0), last_y(0.0),
        ml(false), offset_x(0.0f), offset_y(0.0f), tex() {}
    double yaw, pitch, last_x, last_y; bool ml; float offset_x, offset_y; texture tex;
};

// Helper functions
void register_glfw_callbacks(window& app, state& app_state);
void draw_pointcloud(window& app, state& app_state, rs2::points& points);
*/

int main(int argc, char * argv[]) try
{

    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Post Processing Example");
    ImGui_ImplGlfw_Init(app, false);

    // Taking dimensions of the window for rendering purposes
    float w = static_cast<float>(app.width());
    float h = static_cast<float>(app.height());

    /* Declare two textures on the GPU, one for the original depth image 
    and one for the filtered depth image */
    texture original_image, filtered_image;

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    /*
    // Construct an object to manage view state
    state app_state;
    // register callbacks to allow manipulation of the pointcloud
    register_glfw_callbacks(app, app_state);

    // Declare pointcloud object, for calculating pointclouds and texture mappings
    rs2::pointcloud pc;
    // We want the points object to be persistent so we can display the last cloud when a frame drops
    rs2::points points;
    */

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();


    // decalre filters
    rs2::decimation_filter dec_filter;

    rs2::temporal_filter tem_filter;
    rs2::spatial_filter spat_filter;

    rs2::option_range dec_magnitude_range = dec_filter.get_option_range(RS2_OPTION_FILTER_MAGNITUDE);
    rs2::option_range tem_smooth_alpha_range = tem_filter.get_option_range(RS2_OPTION_FILTER_SMOOTH_ALPHA);
    rs2::option_range tem_smooth_delta_range = tem_filter.get_option_range(RS2_OPTION_FILTER_SMOOTH_DELTA);
    rs2::option_range spat_smooth_alpha_range = spat_filter.get_option_range(RS2_OPTION_FILTER_SMOOTH_ALPHA);
    rs2::option_range spat_smooth_delta_range = spat_filter.get_option_range(RS2_OPTION_FILTER_SMOOTH_DELTA);
    rs2::option_range tem_magnitude_range = tem_filter.get_option_range(RS2_OPTION_FILTER_MAGNITUDE);
    rs2::option_range spat_magnitude_range = spat_filter.get_option_range(RS2_OPTION_FILTER_MAGNITUDE);

    bool do_decimate = false;
    bool do_temporal = false;
    bool do_spatial = false;
    bool disparity = false;

    int decimate_magnitude = dec_magnitude_range.min;
    int temporal_magnitude = tem_magnitude_range.min;    
    int spatial_magnitude = spat_magnitude_range.min;
    float tem_smooth_alpha = tem_smooth_alpha_range.min;
    float tem_smooth_delta = tem_smooth_delta_range.min;
    float spat_smooth_alpha = spat_smooth_alpha_range.min;
    float spat_smooth_delta = spat_smooth_delta_range.min;


    while (app) // Application still alive?
    {
        ImGui_ImplGlfw_NewFrame(1);

        // flags for displaying ImGui windows
        static const int flags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove;

        // Using ImGui library to provide slide controllers for adjusting the filter options
        render_slider_int({ w / 4, h / 2, w / 4, h / 20 }, "slider_dec_mag", "magnitude", &decimate_magnitude, 
                          dec_magnitude_range.min, dec_magnitude_range.max , flags, "Set decimation magnitude");

        render_slider_float({ w / 4, h / 2 + h / 6, w / 4, h / 20 }, "slider_spat_smooth_alpha", "smooth alpha",
                            &spat_smooth_alpha, spat_smooth_alpha_range.min, spat_smooth_alpha_range.max, flags,
                            "Control the weight/radius for smoothing");

        render_slider_float({ w / 4, h / 2 + h / 6 + 50, w / 4, h / 20 }, "slider_spat_smooth_delta", "smooth delta",
            &spat_smooth_delta, spat_smooth_delta_range.min, spat_smooth_delta_range.max, flags,
            "Set filter range/validity threshold");
        
        render_slider_float({ w / 4, h / 2 + h / 3, w / 4, h / 20 }, "slider_tem_smooth_alpha", "smooth alpha",
            &tem_smooth_alpha, tem_smooth_alpha_range.min, tem_smooth_alpha_range.max, flags,
            "Control the weight/radius for smoothing");

        render_slider_float({ w / 4, h / 2 + h / 3 + 50, w / 4, h / 20 }, "slider_tem_smooth_delta", "smooth delta",
            &tem_smooth_delta, tem_smooth_delta_range.min, tem_smooth_delta_range.max, flags,
            "Set filter range/validity threshold");

        // Using Imgui to provide checkboxes for choosing a filter
        render_checkbox({ w / 10, h / 2, w / 10, h / 20 }, &do_decimate, flags, "decimate");
        render_checkbox({ w / 10, h / 2 + h / 6, w / 10, h / 20 }, &do_spatial, flags, "spatial");
        render_checkbox({ w / 10, h / 2 + h / 3, w / 10, h / 20 }, &do_temporal, flags, "temporal");

        render_checkbox({ w / 10, h / 2 + 30, w / 10, h / 20 }, &disparity, flags, "disparity");
        
        ImGui::Render();

        // set the filter options as chosen by the user
        dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, decimate_magnitude);
       // tem_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, temporal_magnitude);
       // spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, spatial_magnitude);
        tem_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, tem_smooth_delta);
        tem_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, tem_smooth_alpha);
        spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, spat_smooth_delta);
        spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, spat_smooth_alpha);

        rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
        rs2::frame depth_frame = data.get_depth_frame(); // find depth frame
        rs2::frame depth = color_map(depth_frame); // colorize the depth data

        // apply filters
        
        if (do_decimate)
            depth_frame = dec_filter.proccess(depth_frame);
       
        if (do_temporal)
            depth_frame = tem_filter.proccess(depth_frame);
       
        if (do_spatial)
            depth_frame = spat_filter.proccess(depth_frame);
        /*
        if (disparity)

        */
        /*
        // Generate the pointcloud and texture mappings
        points = pc.calculate(depth);

        auto color = data.get_color_frame();

        // Tell pointcloud object to map to this color frame
        pc.map_to(color);

        // Upload the color frame to OpenGL
        app_state.tex.upload(color);

        // Draw the pointcloud
        draw_pointcloud(app, app_state, points);
        */

        // Render depth on to the first half of the screen and filtered on to the second
        original_image.render(depth, { 0, 0, w / 2, h / 2 });
        filtered_image.render(color_map(depth_frame), { w / 2, 0, w / 2, h / 2 });
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}


void render_slider_int(rect location,char* name, char* label, int* slider_param,
                       int min, int max, ImGuiWindowFlags flags, char* description) {
    ImGui::SetNextWindowPos({ location.x, location.y });
    ImGui::SetNextWindowSize({ location.w, location.h });
    ImGui::Begin(name, nullptr, flags);
    ImGui::SliderInt("", slider_param, min, max, "", true);
    ImGui::End();
    ImGui::SetNextWindowPos({ location.x + location.w , location.y });
    ImGui::SetNextWindowSize({ location.w, 20 });
    ImGui::Begin(label, nullptr, flags);
    ImGui::TextUnformatted(label);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(description);
    ImGui::End();
}


void render_slider_float(rect location, char* name, char* label, float* slider_param,
                         float min, float max, ImGuiWindowFlags flags, char* description) {
    ImGui::SetNextWindowPos({ location.x, location.y });
    ImGui::SetNextWindowSize({ location.w, location.h });
    ImGui::Begin(name, nullptr, flags);
    ImGui::SliderFloat("", slider_param, min, max, "%.3f", 1.0f, true);
    ImGui::End();
    ImGui::SetNextWindowPos({ location.x + location.w , location.y });
    ImGui::SetNextWindowSize({ location.w, 20 });
    std::string s(name);
    ImGui::Begin((s + "2").c_str(), nullptr, flags);
    ImGui::TextUnformatted(label);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(description);
    ImGui::End();
}


void render_checkbox(rect location, bool* checkbox_param, ImGuiWindowFlags flags, char* label) {
    ImGui::SetNextWindowPos({ location.x, location.y });
    ImGui::SetNextWindowSize({ location.w, location.h });
    ImGui::Begin(label, nullptr, flags);
    ImGui::Checkbox(label, checkbox_param);
    ImGui::End();
}


/*
// Registers the state variable and callbacks to allow mouse control of the pointcloud
void register_glfw_callbacks(window& app, state& app_state)
{
    app.on_left_mouse = [&](bool pressed)
    {
        app_state.ml = pressed;
    };

    app.on_mouse_scroll = [&](double xoffset, double yoffset)
    {
        app_state.offset_x += static_cast<float>(xoffset);
        app_state.offset_y += static_cast<float>(yoffset);
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

// Handles all the OpenGL calls needed to display the point cloud
void draw_pointcloud(window& app, state& app_state, rs2::points& points)
{
    if (!points)
        return;

    // OpenGL commands that prep screen for the pointcloud
    glPopMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    float width = app.width(), height = app.height();

    glClearColor(153.f / 255, 153.f / 255, 153.f / 255, 1);
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
    */

    /* this segment actually prints the pointcloud */
/*
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
*/
