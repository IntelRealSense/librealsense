// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

#include <map>
#include <thread>

#include <imgui.h>
#include "imgui_impl_glfw.h"


/* Struct that holds a pointer to an rs2_option, we'll store a pointer to a filter this way;
int_params and float_params: maps from an option supported by the filter, to the variable that controls
this option;
string: map from an option supported by the filter, to the corresponding vector of strings that will
be used by the gui;
do_filter: a boolean controlled by the user that determines whether to apply the filter.*/
struct filter_options {
    rs2::options* filter;
    std::map<rs2_option, int> int_params;
    std::map<rs2_option, float> float_params;
    std::map<rs2_option, std::vector<char*> > string;
    bool do_filter;
};

struct state
{
    state() : yaw(0.0), tex() {}
    double yaw; texture tex;
};

// helper functions for rendering the ui
void render_slider_int(const float3 location, char* name, char* label, int* slider_param,
    int min, int max, char* description, std::mutex& lock);
void render_slider_float(const float3 location, char* name, char* label, float* slider_param,
    float min, float max, char* description, std::mutex& lock);
void render_checkbox(const float2 location, bool* checkbox_param, char* label, std::mutex& lock);
void render_ui(float w, float h, std::vector<filter_options>& filters, std::mutex& param_lock,
    const std::vector<rs2_option>& option_names);
void save_strings(filter_options& dec_struct, filter_options& spat_struct, filter_options& temp_struct);
bool is_all_integers(float min, float max, float def, float step);

inline bool is_integer(float f)
{
    return (fabs(fmod(f, 1)) < std::numeric_limits<float>::min());
}

// helper functions to handle filter options
void set_defaults(std::vector<filter_options>& filters, const std::vector<rs2_option>& option_names);
void set_filter_options(std::vector<filter_options>& filters, std::mutex& param_lock,
     const std::vector<rs2_option>& option_names);

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

    /* Declare pointcloud objects, for calculating pointclouds and texture mappings
    one is for the original frames and the second is for the filtered ones */
    rs2::pointcloud pc;
    rs2::pointcloud pc_filtered;

    /* Declare objects that will hold the calculated pointclouds, one is for the
    original and one for filtered */
    rs2::points depth_points;
    rs2::points filtered_points;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    // save the the time of last frame's arrival
    auto last_time = std::chrono::high_resolution_clock::now();
    // a time interval which must pass between movement of the pointcloud
    const std::chrono::milliseconds rotation_delta(40);
    // maximum angle for the rotation of the pointcloud
    const int max_angle = 90;
    // we'll use velocity to rotate the pointcloud for a better view of the filters effects
    float velocity = 0.3;

    /* Decalre filters: decimation, temporal and spatial
     Decimation - reduces depth frame density; Spatial - edge-preserving spatial smoothing;
    Temporal - reduces temporal noise */
    rs2::decimation_filter dec_filter;
    rs2::spatial_filter spat_filter;
    rs2::temporal_filter temp_filter;

    // Declare disparity transform from depth to disparity and vice versa
    rs2::disparity_transform depth_to_disparity;
    rs2::disparity_transform disparity_to_depth(false);

    // initialize a vector that would allow iterating on possible filter options
    std::vector<rs2_option> option_names = { RS2_OPTION_FILTER_MAGNITUDE, RS2_OPTION_FILTER_SMOOTH_ALPHA,
        RS2_OPTION_FILTER_SMOOTH_DELTA };
    
    // initialize a vector that holds filters and their option values
    filter_options dec_struct;
    dec_struct.filter = &dec_filter;
    filter_options disparity_struct;
    disparity_struct.filter = &depth_to_disparity;
    filter_options spat_struct;
    spat_struct.filter = &spat_filter;
    filter_options temp_struct;
    temp_struct.filter = &temp_filter;
    save_strings(dec_struct, spat_struct, temp_struct);
    std::vector<filter_options> filters = { dec_struct, disparity_struct, spat_struct, temp_struct };

    // set parameters that are controlled by the user to default values
    set_defaults(filters, option_names);

    rs2::frame_queue depth_queue;
    rs2::frame_queue filtered_queue;

    // mutex to synchronize access to user-contorlled parameters that are used to set the filter options
    std::mutex param_lock;
    //std::mutex pc_lock;
    std::mutex points_lock;

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    std::thread t([&]() {
        while (app) {
            rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
            rs2::frame depth_frame = data.get_depth_frame(); // find depth frame
            rs2::frame filtered = depth_frame; // declare frame to apply filters on, while preserving the original depth data

            std::unique_lock<std::mutex> ul1(param_lock);
            bool do_decimate = filters[0].do_filter;
            bool do_disparity = filters[1].do_filter;
            bool do_spatial = filters[2].do_filter;
            bool do_temporal = filters[3].do_filter;
            ul1.unlock();

            /* Apply filters. The implemented flow of the filters pipeline is in the following order: apply decimation filter,
            transform the scence into disparity domain, apply spatial filter, apply temporal filter, revert the results back
            to depth domain (each post processing block is optional and can be applied independantly).
            */
            if (do_decimate)
                filtered = static_cast<rs2::decimation_filter*>(filters[0].filter)->process(filtered);

            if (do_disparity)
                filtered = static_cast<rs2::disparity_transform*>(filters[1].filter)->process(filtered);

            if (do_spatial)
                filtered = static_cast<rs2::spatial_filter*>(filters[2].filter)->process(filtered);

            if (do_temporal)
                filtered = static_cast<rs2::temporal_filter*>(filters[3].filter)->process(filtered);

            if (do_disparity)
                filtered = disparity_to_depth.process(filtered);


            // Generate the pointcloud and texture mappings
            std::unique_lock<std::mutex> ul2(points_lock);
            depth_points = pc.calculate(depth_frame);
            ul2.unlock();
            std::unique_lock<std::mutex> ul3(points_lock);
            filtered_points = pc_filtered.calculate(filtered);
            ul3.unlock();

            // Colorize frames to map to the pointclouds
            rs2::frame colored_depth = color_map(depth_frame);
            rs2::frame colored_filtered = color_map(filtered);

            depth_queue.enqueue(colored_depth);
            filtered_queue.enqueue(colored_filtered);
        }
    });
    
    // Frame objects to hold ready frames
    rs2::frame colored_filtered;
    rs2::frame colored_depth;

    while (app) {
        float w = static_cast<float>(app.width());
        float h = static_cast<float>(app.height());

        // render the user interface: sliders, checkboxes
        render_ui(w, h, filters, param_lock, option_names);

        // Set the filter options as chosen by the user (or by default)
        set_filter_options(filters, param_lock, option_names);

        // Tell each pointcloud object to map to to the colorized frame and upload the colorized frames to OpenGL
        if (depth_queue.poll_for_frame(&colored_depth)) {
            pc.map_to(colored_depth);
            app_state.tex.upload(colored_depth);
            
            // wait for the filtered frame
            colored_filtered = filtered_queue.wait_for_frame();
            pc_filtered.map_to(colored_filtered);
            app_state_filtered.tex.upload(colored_filtered);
        }

        // Draw the pointclouds of the original and the filtered frames    
        std::unique_lock<std::mutex> ul2(points_lock);
        draw_pointcloud({ 0, h / 2, w / 2, h / 2 }, app_state, depth_points);
        ul2.unlock();
        std::unique_lock<std::mutex> ul3(points_lock);
        draw_pointcloud({ w / 2, h / 2, w / 2, h / 2 }, app_state_filtered, filtered_points);
        ul3.unlock();

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

// helper function for deciding on int ot float slider
bool is_all_integers(float min, float max, float def, float step)
{
    return is_integer(min) && is_integer(max) &&
        is_integer(def) && is_integer(step);
}


// Set default values for the parameters that control filter options through the interface
void set_defaults(std::vector<filter_options>& filters, const std::vector<rs2_option>& option_names) {
    for (int i = 0; i < filters.size(); i++) {
        for (int j = 0; j < option_names.size(); j++) {
            if (filters[i].filter->supports(option_names[j])) {
                rs2::option_range range = filters[i].filter->get_option_range(option_names[j]);
                if (is_all_integers(range.min, range.max, range.def, range.step))
                {
                    filters[i].int_params[option_names[j]] = range.def;
                    int value = filters[i].int_params[option_names[j]];
                }
                else
                {
                    float value = filters[i].int_params[option_names[j]];
                    filters[i].float_params[option_names[j]] = range.def;
                }
            }
        }
        filters[i].do_filter = true;
    }
}


// Iterate over all filters, and set every supported option with the chosen value
void set_filter_options(std::vector<filter_options>& filters, std::mutex& param_lock, 
    const std::vector<rs2_option>& option_names) {
    for (int i = 0; i < filters.size(); i++) {
        for (int j = 0; j < option_names.size(); j++) {
            if (filters[i].filter->supports(option_names[j])) {
                rs2::option_range range = filters[i].filter->get_option_range(option_names[j]);
                if (is_all_integers(range.min, range.max, range.def, range.step))
                {
                    std::lock_guard<std::mutex> lock(param_lock);
                    filters[i].filter->set_option(option_names[j], filters[i].int_params[option_names[j]]);
                }
                else
                {
                    std::lock_guard<std::mutex> lock(param_lock);
                    filters[i].filter->set_option(option_names[j], filters[i].float_params[option_names[j]]);
                }
            }
        }
    }
}

void render_ui(float w, float h, std::vector<filter_options>& filters, std::mutex& param_lock,
    const std::vector<rs2_option>& option_names) {
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
    std::vector<char*> checkbox_names{ "Decimate", "Disparity", "Spatial", "Temporal" };
    int x = 20, y = 10;
    for (int i = 0; i < filters.size(); i++) {
         for (int j = 0; j < option_names.size(); j++) {
            if (filters[i].filter->supports(option_names[j])) {
                rs2::option_range range = filters[i].filter->get_option_range(option_names[j]);
                if (is_all_integers(range.min, range.max, range.def, range.step))
                {
                    render_slider_int({ w / 4 + dist, h / 2 + x, w / 4 }, filters[i].string[option_names[j]][0],
                        filters[i].string[option_names[j]][1], &(filters[i].int_params[option_names[j]]), range.min, range.max,
                        filters[i].string[option_names[j]][2], param_lock);
                    x = x + 50;
                }
                else
                {
                    render_slider_float({ w / 4 + dist, h / 2 + x, w / 4 }, filters[i].string[option_names[j]][0],
                        filters[i].string[option_names[j]][1], &(filters[i].float_params[option_names[j]]),
                        range.min, range.max, filters[i].string[option_names[j]][2], param_lock);
                    x += 50;
                }
            }
        }
        // Using Imgui to provide checkboxes for choosing a filter
        render_checkbox({ w / 4, h / 2 + 10 + y }, &filters[i].do_filter, checkbox_names[i], param_lock);
        y += 30;
        if (i == 1)
            x += 12;
        if (i == 2)
            y += 122;
    }

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

void save_strings(filter_options& dec_struct, filter_options& spat_struct, filter_options& temp_struct) {
    dec_struct.string[RS2_OPTION_FILTER_MAGNITUDE] = { "slider_dec_mag", "Magnitude",
        "Decimation filter magnitude" };
    spat_struct.string[RS2_OPTION_FILTER_MAGNITUDE] = { "slider_spat_mag", "Magnitude",
        "Spatial filter magnitude" };
    spat_struct.string[RS2_OPTION_FILTER_SMOOTH_ALPHA] = { "slider_spat_smooth_alpha", "Smooth Alpha",
        "Current pixel weight" };
    spat_struct.string[RS2_OPTION_FILTER_SMOOTH_DELTA] = { "slider_spat_smooth_delta", "Smooth Delta",
        "Convolution radius" };
    temp_struct.string[RS2_OPTION_FILTER_MAGNITUDE] = { "slider_tem_mag", "Magnitude",
        "Temporal filter magnitude" };
    temp_struct.string[RS2_OPTION_FILTER_SMOOTH_ALPHA] = { "slider_tem_smooth_alpha", "Smooth Alpha",
        "The normalized weight of the current pixel" };
    temp_struct.string[RS2_OPTION_FILTER_SMOOTH_DELTA] = { "slider_tem_smooth_delta", "Smooth Delta",
        "Depth range (gradient) threshold" };
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

    glTranslatef(0, 0, +0.5f);
    glRotated(app_state.yaw, 0, 1, 0);
    glTranslatef(0, 0, -0.5f);

    glPointSize(width / 640);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, app_state.tex.get_gl_handle());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); // GL_CLAMP_TO_EDGE
    glBegin(GL_POINTS);

    // this segment actually prints the pointcloud
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

