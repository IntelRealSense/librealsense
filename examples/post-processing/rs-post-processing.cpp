// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

#include <map>
#include <thread>

#include <imgui.h>
#include "imgui_impl_glfw.h"


struct state
{
    state() : yaw(0.0), tex() {}
    double yaw; texture tex;
};

// Struct of parameters controled by the user
struct params
{
    bool do_decimate, do_spatial, do_temporal, do_disparity;
    int decimate_magnitude, temporal_magnitude, spatial_magnitude, temp_smooth_delta, spat_smooth_delta;
    float temp_smooth_alpha, spat_smooth_alpha;
};

struct filter_ranges
{
    rs2::option_range dec_magnitude_range, spat_magnitude_range, spat_smooth_alpha_range,
        spat_smooth_delta_range, temp_smooth_alpha_range, temp_smooth_delta_range, temp_magnitude_range;
};

// helper functions for rendering the ui
void render_slider_int(const float3 location, char* name, char* label, int* slider_param,
    int min, int max, char* description, std::mutex& lock);
void render_slider_float(const float3 location, char* name, char* label, float* slider_param,
    float min, float max, char* description, std::mutex& lock);
void render_checkbox(const float2 location, bool* checkbox_param, char* label, std::mutex& lock);
void render_ui(float w, float h, params& params, const filter_ranges& ranges, std::mutex& lock);


// helper functions to handle filter options
void get_filter_ranges(filter_ranges& ranges, rs2::decimation_filter& dec_filter,
    rs2::spatial_filter& spat_filter, rs2::temporal_filter& temp_filter);
void set_defaults(params& params, filter_ranges& ranges);
void set_filter_options(params& params, rs2::decimation_filter& dec_filter,
    rs2::spatial_filter& spat_filter, rs2::temporal_filter& temp_filter, std::mutex& lock);

// A function that draws the 3D-display
void draw_pointcloud(const rect& viewer_rect, state& app_state, rs2::points& points);


int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Post Processing Example");
    ImGui_ImplGlfw_Init(app, false);

    // Construct objects to manage view state
    state app_state;
    state app_state_filtered;

    // Declare pointcloud objects, for calculating pointclouds and texture mappings
    //one is for the original frames and the second is for the filtered ones
    rs2::pointcloud pc;
    rs2::pointcloud pc_filtered;

    // Declare objects that will hold the calculated pointclouds, one is for the
    //original and one for filtered
    rs2::points depth_points;
    rs2::points filtered_points;

    // save the the time of last frame's arrival
    auto last_time = std::chrono::high_resolution_clock::now();
    // a time interval which must pass between movement of the pointcloud
    const std::chrono::milliseconds rotation_delta(10);
    // maximum angle for the rotation of the pointcloud
    const int max_angle = 90;
    // we'll use velocity to rotate the pointcloud for a better view of the filters effects
    float velocity = 0.3;

    texture depth_image, color_image;

    // Decalre filters: decimation, temporal and spatial
    // decimation - reduces depth frame density;
    // spatial - edge-preserving spatial smoothing
    // temporal - reduces temporal noise
    rs2::decimation_filter dec_filter;
    rs2::spatial_filter spat_filter;
    rs2::temporal_filter temp_filter;

    // Declare disparity transform from depth to disparity and vice versa
    rs2::disparity_transform depth_to_disparity;
    rs2::disparity_transform disparity_to_depth(false);

    params params;

    filter_ranges ranges;

    // get ranges of all filters
    get_filter_ranges(ranges, dec_filter, spat_filter, temp_filter);

    // set filters to defaults values
    set_defaults(params, ranges);

    rs2::frame_queue depth_queue;
    rs2::frame_queue filtered_queue;
    //rs2::frame_queue filtered_original_queue;
    //rs2::frame_queue depth_original_queue;

    std::mutex param_lock;
    std::mutex pc_lock;
    std::mutex points_lock;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    std::thread t([&]() {

        while (app) {


            rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
            rs2::frame depth_frame = data.get_depth_frame(); // find depth frame
            rs2::frame filtered = depth_frame; // declare frame to apply filters on, while preserving the original depth data


           // std::unique_lock<std::mutex> ul(param_lock);

            // Set the filter options as chosen by the user (or by default)
            set_filter_options(params, dec_filter, spat_filter, temp_filter, param_lock);
            // ul.unlock();

            std::unique_lock<std::mutex> ul1(param_lock);
            // Apply filters. The implemented flow of the filters pipeline is in the following order: apply decimation filter,
            //transform the scence into disparity domain, apply spatial filter, apply temporal filter, revert the results back
            //to depth domain (each post processing block is optional and can be applied independantly).
            //
            if (params.do_decimate)
                filtered = dec_filter.proccess(filtered);
            ul1.unlock();

            std::unique_lock<std::mutex> ul2(param_lock);
            if (params.do_disparity)
                filtered = depth_to_disparity.proccess(filtered);
            ul2.unlock();

            std::unique_lock<std::mutex> ul3(param_lock);
            if (params.do_spatial)
                filtered = spat_filter.proccess(filtered);
            ul3.unlock();

            std::unique_lock<std::mutex> ul4(param_lock);
            if (params.do_temporal)
                filtered = temp_filter.proccess(filtered);
            ul4.unlock();

            std::unique_lock<std::mutex> ul5(param_lock);
            if (params.do_disparity)
                filtered = disparity_to_depth.proccess(filtered);
            ul5.unlock();

            //depth_original_queue.enqueue(depth_frame);
            //filtered_original_queue.enqueue(filtered);


            std::unique_lock<std::mutex> ul6(points_lock);

            // Generate the pointcloud and texture mappings
            depth_points = pc.calculate(depth_frame);
            filtered_points = pc_filtered.calculate(filtered);

            ul6.unlock();

            // Colorize frames to map to the pointclouds

            rs2::frame colored_depth = color_map(depth_frame);

            depth_queue.enqueue(colored_depth);

            rs2::frame colored_filtered = color_map(filtered);

            filtered_queue.enqueue(colored_filtered);
        }
    });

    //rs2::frame depth_frame;
    //rs2::frame filtered;
    rs2::frame colored_filtered;
    rs2::frame colored_depth;

    while (app) // Application still alive?
    {

        // Taking dimensions of the window for rendering purposes
        float w = static_cast<float>(app.width());
        float h = static_cast<float>(app.height());

       // std::unique_lock<std::mutex> ul1(param_lock);

        // render the user interface: sliders, checkboxes
        render_ui(w, h, params, ranges, param_lock);

        //ul1.unlock();
        /*
        if (depth_original_queue.poll_for_frame(&depth_frame)) {
        filtered = filtered_original_queue.wait_for_frame();

        // Generate the pointcloud and texture mappings
        depth_points = pc.calculate(depth_frame);
        filtered_points = pc_filtered.calculate(filtered);

        }
        */
        if (depth_queue.poll_for_frame(&colored_depth)) {
            pc.map_to(colored_depth);
            app_state.tex.upload(colored_depth);

            if (filtered_queue.poll_for_frame(&colored_filtered)) {
                // Tell each pointcloud object to map to to the colorized frame
                // std::unique_lock<std::mutex> ul(pc_lock);

                pc_filtered.map_to(colored_filtered);
                //    ul.unlock();

                // Upload the colorized frames to OpenGL

                app_state_filtered.tex.upload(colored_filtered);

            }
        }


        std::unique_lock<std::mutex> ul2(points_lock);
        // Draw the pointclouds of the original and the filtered frames    
        draw_pointcloud({ 0, h / 2, w / 2, h / 2 }, app_state, depth_points);
        draw_pointcloud({ w / 2, h / 2, w / 2, h / 2 }, app_state_filtered, filtered_points);
        ul2.unlock();

        // Update time of current frame's arrival
        auto curr = std::chrono::high_resolution_clock::now();

        // In order to calibrate the velocity of the rotation to the actual displaying speed, rotate
        //pointcloud only when enough time has passed between frames
        if (curr - last_time > rotation_delta)
        {
            if (fabs(app_state_filtered.yaw) > max_angle)
            {
                velocity = -velocity;
            }
            app_state.yaw += velocity;
            app_state_filtered.yaw += velocity;
            last_time = curr;
        }
    }

    t.join();

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


/* Get the range of each filter option, which contains minimum, maximum and
default values for that option, as well as the step */

void get_filter_ranges(filter_ranges& ranges, rs2::decimation_filter& dec_filter,
    rs2::spatial_filter& spat_filter, rs2::temporal_filter& temp_filter) {
    ranges.dec_magnitude_range = dec_filter.get_option_range(RS2_OPTION_FILTER_MAGNITUDE);
    ranges.spat_magnitude_range = spat_filter.get_option_range(RS2_OPTION_FILTER_MAGNITUDE);
    ranges.spat_smooth_alpha_range = spat_filter.get_option_range(RS2_OPTION_FILTER_SMOOTH_ALPHA);
    ranges.spat_smooth_delta_range = spat_filter.get_option_range(RS2_OPTION_FILTER_SMOOTH_DELTA);
    ranges.temp_smooth_alpha_range = temp_filter.get_option_range(RS2_OPTION_FILTER_SMOOTH_ALPHA);
    ranges.temp_smooth_delta_range = temp_filter.get_option_range(RS2_OPTION_FILTER_SMOOTH_DELTA);
    ranges.temp_magnitude_range = temp_filter.get_option_range(RS2_OPTION_FILTER_MAGNITUDE);
}


void set_defaults(params& params, filter_ranges& ranges) {
    // Enable all filters by default
    params.do_decimate = true;
    params.do_temporal = true;
    params.do_spatial = true;
    params.do_disparity = true;

    // Set default values for all filters
    params.decimate_magnitude = ranges.dec_magnitude_range.def;
    params.temporal_magnitude = ranges.temp_magnitude_range.def;
    params.spatial_magnitude = ranges.spat_magnitude_range.def;
    params.temp_smooth_alpha = ranges.temp_smooth_alpha_range.def;
    params.temp_smooth_delta = ranges.temp_smooth_delta_range.def;
    params.spat_smooth_alpha = ranges.spat_smooth_alpha_range.def;
    params.spat_smooth_delta = ranges.spat_smooth_delta_range.def;
}


void set_filter_options(params& params, rs2::decimation_filter& dec_filter,
    rs2::spatial_filter& spat_filter, rs2::temporal_filter& temp_filter, std::mutex& lock) {
    dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, params.decimate_magnitude);
    spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, params.spatial_magnitude);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, params.spat_smooth_alpha);
    spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, params.spat_smooth_delta);
    temp_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, params.temporal_magnitude);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, params.temp_smooth_alpha);
    temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, params.temp_smooth_delta);
}

void render_ui(float w, float h, params& params, const filter_ranges& ranges, std::mutex& lock) {
    // for the interface: distance between checkbox and slider
    const int dist = 120;

    // Flags for displaying ImGui window
    static const int flags = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove;

    ImGui_ImplGlfw_NewFrame(1);
    ImGui::SetNextWindowSize({ w, h });
    ImGui::Begin("app", nullptr, flags);

    // Using ImGui library to provide slide controllers for adjusting the filter options
    render_slider_int({ w / 4 + dist, h / 2 + 10, w / 4 }, "slider_dec_mag", "Magnitude", &params.decimate_magnitude,
        ranges.dec_magnitude_range.min, ranges.dec_magnitude_range.max, "Decimation filter magnitude", lock);

    render_slider_int({ w / 4 + dist, h / 2 + 70, w / 4 }, "slider_spat_mag", "Magnitude",
        &params.spatial_magnitude, ranges.spat_magnitude_range.min, ranges.spat_magnitude_range.max,
        "Spatial filter magnitude", lock);

    render_slider_float({ w / 4 + dist, h / 2 + 110, w / 4 }, "slider_spat_smooth_alpha", "Smooth Alpha",
        &params.spat_smooth_alpha, ranges.spat_smooth_alpha_range.min, ranges.spat_smooth_alpha_range.max,
        "Current pixel weight", lock);

    render_slider_int({ w / 4 + dist, h / 2 + 150, w / 4 }, "slider_spat_smooth_delta", "Smooth Delta",
        &params.spat_smooth_delta, ranges.spat_smooth_delta_range.min, ranges.spat_smooth_delta_range.max,
        "Convolution radius", lock);

    render_slider_int({ w / 4 + dist, h / 2 + 210, w / 4 }, "slider_tem_mag", "Magnitude",
        &params.temporal_magnitude, ranges.temp_magnitude_range.min, ranges.temp_magnitude_range.max,
        "Temporal filter magnitude", lock);

    render_slider_float({ w / 4 + dist, h / 2 + 250, w / 4 }, "slider_tem_smooth_alpha", "Smooth Alpha",
        &params.temp_smooth_alpha, ranges.temp_smooth_alpha_range.min, ranges.temp_smooth_alpha_range.max,
        "The normalized weight of the current pixel", lock);

    render_slider_int({ w / 4 + dist, h / 2 + 290, w / 4 }, "slider_tem_smooth_delta", "Smooth Delta",
        &params.temp_smooth_delta, ranges.temp_smooth_delta_range.min, ranges.temp_smooth_delta_range.max,
        "Depth range (gradient) threshold", lock);

    // Using Imgui to provide checkboxes for choosing a filter
    render_checkbox({ w / 4, h / 2 + 10 }, &params.do_decimate, "Decimate", lock);
    render_checkbox({ w / 4, h / 2 + 40 }, &params.do_disparity, "Disparity", lock);
    render_checkbox({ w / 4, h / 2 + 70 }, &params.do_spatial, "Spatial", lock);
    render_checkbox({ w / 4, h / 2 + 210 }, &params.do_temporal, "Temporal", lock);

    // Adding before and after labels
    ImGui::SetCursorPosX(20);
    ImGui::SetCursorPosY(10);
    ImGui::TextUnformatted("Before");

    ImGui::SetCursorPosX(w / 2 + 20);
    ImGui::SetCursorPosY(10);
    ImGui::TextUnformatted("After");

    ImGui::End();
    ImGui::Render();
}

/*
Function that renders the slider. Parameter: location: x, y - coordinates for the text, z - width of the slider,
name: used as a unique id for the slider, label: the text to display next to the slider,
slider_param: pointer to the parameter controled by the slider, min, max: lower and upper bounds of the slider,
description: text to display when mouse is hovered on the slider label.
*/
void render_slider_int(const float3 location, char* name, char* label, int* slider_param,
    int min, int max, char* description, std::mutex& lock) {
    ImGui::PushItemWidth(location.z);
    ImGui::SetCursorPosX(location.x + 100);
    ImGui::SetCursorPosY(location.y);
    std::string s(name);

    std::unique_lock<std::mutex> ul1(lock);
    ImGui::SliderInt(("##" + s).c_str(), slider_param, min, max, "%.0f");
    ul1.unlock();

    ImGui::PopItemWidth();
    ImGui::SetCursorPosX(location.x);
    ImGui::SetCursorPosY(location.y + 3);
    ImGui::TextUnformatted(label);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(description);
}


void render_slider_float(const float3 location, char* name, char* label, float* slider_param,
    float min, float max, char* description, std::mutex& lock) {
    ImGui::PushItemWidth(location.z);
    ImGui::SetCursorPosX(location.x + 100);
    ImGui::SetCursorPosY(location.y);
    std::string s(name);

    std::unique_lock<std::mutex> ul1(lock);
    ImGui::SliderFloat(("##" + s).c_str(), slider_param, min, max, "%.3f", 1.0f);
    ul1.unlock();

    ImGui::PopItemWidth();
    ImGui::SetCursorPosX(location.x);
    ImGui::SetCursorPosY(location.y + 3);
    ImGui::TextUnformatted(label);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(description);
}

/*
Function that renders the checkbox. Parameters: location: x, y coordinates for the checkbox,
checkbox_param: pointer to the parameter controled by the checkbox,
label: a string that appears next to the checkbox.
*/
void render_checkbox(const float2 location, bool* checkbox_param, char* label, std::mutex& lock) {
    ImGui::SetCursorPosX(location.x);
    ImGui::SetCursorPosY(location.y);
    std::unique_lock<std::mutex> ul1(lock);
    ImGui::Checkbox(label, checkbox_param);
    ul1.unlock();
}


// Handles all the OpenGL calls needed to display the point cloud
void draw_pointcloud(const rect& viewer_rect, state& app_state, rs2::points& points)
{
    if (!points)
        return;

    glViewport(viewer_rect.x, viewer_rect.y,
        viewer_rect.w, viewer_rect.h);

    // OpenGL commands that prep screen for the pointcloud
    glPopMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    float width = viewer_rect.w, height = viewer_rect.h;

    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    gluPerspective(60, width / height, 0.01f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    gluLookAt(0, 0, 0, 0, 0, 1, 0, -1, 0);

    // glTranslatef(0, 0, +0.5f + app_state.offset_y*0.05f);
    glTranslatef(0, 0, +0.5f);
    // glRotated(app_state.pitch, 1, 0, 0);
    glRotated(app_state.yaw, 0, 1, 0);
    glTranslatef(0, 0, -0.5f);

    glPointSize(width / 640);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, app_state.tex.get_gl_handle());
    // float tex_border_color[] = { 0.8f, 0.8f, 0.8f, 0.8f };

    // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); // GL_CLAMP_TO_EDGE
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

