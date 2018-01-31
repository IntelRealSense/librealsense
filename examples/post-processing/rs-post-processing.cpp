// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

#include <map>
#include <thread>

#include <imgui.h>
#include "imgui_impl_glfw.h"

/**
    Helper class for controlling the filter's GUI element
*/
struct filter_slider_ui
{
    std::string name;
    std::string label;
    std::string description;
    bool is_int;
    float value;
    rs2::option_range range;
    bool render(const float3& location, bool enabled);
};

/**
    Class to encapsulate a filter alongside its GUI elements
*/
class filter_options
{
public:
    filter_options(const std::string name, rs2::process_interface& filter);
    filter_options(filter_options&& other);
    std::string filter_name;                                   //Friendly name of the filter
    rs2::process_interface& filter;                            //The filter in use
    std::map<rs2_option, filter_slider_ui> supported_options;  //maps from an option supported by the filter, to the corresponding slider
    std::atomic_bool is_enabled;                               //A boolean controlled by the user that determines whether to apply the filter or not
private:
    // helper function for deciding on int ot float slider
    bool is_all_integers(const rs2::option_range& range);
};

// Helper functions for rendering the ui
void render_checkbox(const float2 location, bool* checkbox_param, const char* label);
void render_ui(float w, float h, std::vector<filter_options>& filters);

int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Post Processing Example");
    ImGui_ImplGlfw_Init(app, false);

    // Construct objects to manage view state
    glfw_state original_view_orientation{};
    glfw_state filtered_view_orientation{};

    // Declare pointcloud objects, for calculating pointclouds and texture mappings
    rs2::pointcloud original_pc;
    rs2::pointcloud filtered_pc;

    // Declare objects that will hold the calculated pointclouds
    rs2::points original_points;
    rs2::points filtered_points;


    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
    // Start streaming with default recommended configuration
    pipe.start(cfg);

    // save the time of last frame's arrival
    auto last_time = std::chrono::high_resolution_clock::now();
    // maximum angle for the rotation of the pointcloud
    const double max_angle = 90.0;
    // we'll use velocity to rotate the pointcloud for a better view of the filters effects
    float velocity = 0.3f;

    // Decalre filters
    rs2::decimation_filter dec_filter;  // Decimation - reduces depth frame density
    rs2::spatial_filter spat_filter;    // Spatial    - edge-preserving spatial smoothing
    rs2::temporal_filter temp_filter;   // Temporal   - reduces temporal noise

    // Declare disparity transform from depth to disparity and vice versa
    const std::string disparity_filter_name = "Disparity";
    rs2::disparity_transform depth_to_disparity(true);
    rs2::disparity_transform disparity_to_depth(false);

    // initialize a vector that holds filters and their option values
    std::vector<filter_options> filters;
    filters.emplace_back("Decimate", dec_filter);
    filters.emplace_back(disparity_filter_name, depth_to_disparity);
    filters.emplace_back("Spatial", spat_filter);
    filters.emplace_back("Temporal", temp_filter);

    rs2::frame_queue depth_queue;
    rs2::frame_queue filtered_queue;

    // mutex to synchronize access to user-contorlled parameters that are used to set the filter options
    std::mutex points_lock;

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    std::thread t([&]() {
        while (app) {
            rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
            rs2::frame depth_frame = data.get_depth_frame(); // find depth frame
            rs2::frame filtered = depth_frame; // declare frame to apply filters on, while preserving the original depth data

            /* Apply filters.
                The implemented flow of the filters pipeline is in the following order: 
                    1. apply decimation filter
                    2. transform the scence into disparity domain
                    3. apply spatial filter
                    4. apply temporal filter
                    5. revert the results back (if step Disparity filter was applied
            to depth domain (each post processing block is optional and can be applied independantly).
            */
            bool revert_disparity = false;
            for (auto&& filter : filters)
            {
                if (filter.is_enabled)
                {
                    filtered = filter.filter.process(filtered);
                    if (filter.filter_name == disparity_filter_name)
                    {
                        revert_disparity = true;
                    }
                }
            }

            if (revert_disparity)
                filtered = disparity_to_depth.process(filtered);


            // Generate the pointcloud and texture mappings
            std::unique_lock<std::mutex> ul2(points_lock);
            original_points = original_pc.calculate(depth_frame);
            ul2.unlock();
            std::unique_lock<std::mutex> ul3(points_lock);
            filtered_points = filtered_pc.calculate(filtered);
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

        // Render the GUI
        render_ui(w, h, filters);

        // Tell each pointcloud object to map to to the colorized frame and upload the colorized frames to OpenGL
        if (depth_queue.poll_for_frame(&colored_depth)) {
            original_pc.map_to(colored_depth);
            original_view_orientation.tex.upload(colored_depth);
            
            // wait for the filtered frame
            colored_filtered = filtered_queue.wait_for_frame();
            filtered_pc.map_to(colored_filtered);
            filtered_view_orientation.tex.upload(colored_filtered);
        }

        // Draw the pointclouds of the original and the filtered frames    
        std::unique_lock<std::mutex> ul2(points_lock);
        draw_text(10, 50, "Original");
        draw_text(static_cast<int>(w / 2), 50, "Filtered");
        glViewport(0, h / 2, w / 2, h / 2);
        draw_pointcloud(w / 2, h / 2, original_view_orientation, original_points);
        ul2.unlock();
        std::unique_lock<std::mutex> ul3(points_lock);
        glViewport(w / 2, h / 2, w / 2, h / 2);
        draw_pointcloud(w / 2, h / 2, filtered_view_orientation, filtered_points);
        ul3.unlock();

        // Update time of current frame's arrival
        auto curr = std::chrono::high_resolution_clock::now();

        // a time interval which must pass between movement of the pointcloud
        const std::chrono::milliseconds rotation_delta(40);

        // In order to calibrate the velocity of the rotation to the actual displaying speed, rotate
        //pointcloud only when enough time has passed between frames
        if (curr - last_time > rotation_delta)
        {
            if (std::fabs(filtered_view_orientation.yaw) > max_angle)
            {
                velocity = -velocity;
            }
            original_view_orientation.yaw += velocity;
            filtered_view_orientation.yaw += velocity;
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

void render_ui(float w, float h, std::vector<filter_options>& filters) {
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
    int x = 20, y = 10;
    int i = 0;
    for (auto& filter : filters)
    {
        for (auto& option_slider_pair : filter.supported_options)
        {
            filter_slider_ui& slider = option_slider_pair.second;
            if (slider.render({ w / 4 + dist, h / 2 + x, w / 4 }, filter.is_enabled))
            {
                filter.filter.set_option(option_slider_pair.first, slider.value);
            }
            x += 50;
        }
        // Using Imgui to provide checkboxes for choosing a filter
        bool tmp_value = filter.is_enabled;
        render_checkbox({ w / 4, h / 2 + 10 + y }, &tmp_value, filter.filter_name.c_str());
        filter.is_enabled = tmp_value;
        y += 30;
        if (i == 1)
            x += 12;
        if (i == 2)
            y += 122;

        i++;
    }

    ImGui::End();
    ImGui::Render();
}

void render_checkbox(const float2 location, bool* checkbox_param, const char* label)
{
    ImGui::SetCursorPosX(location.x);
    ImGui::SetCursorPosY(location.y);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, { 40 / 255.f, 170 / 255.f, 90 / 255.f, 1 });
    ImGui::Checkbox(label, checkbox_param);
    ImGui::PopStyleColor();
}

bool filter_slider_ui::render(const float3& location, bool enabled)
{
    bool value_changed = false;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, { 40 / 255.f, 170 / 255.f, 90 / 255.f, 1 });
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, { 20 / 255.f, 150 / 255.f, 70 / 255.f, 1 });
    ImGui::GetStyle().GrabRounding = 12;
    if (!enabled)
    {
        ImGui::PushStyleColor(ImGuiCol_SliderGrab, { 0,0,0,0 });
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, { 0,0,0,0 });
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_TextDisabled)));
    }

    ImGui::PushItemWidth(location.z);
    ImGui::SetCursorPos({ location.x, location.y + 3 });
    ImGui::TextUnformatted(label.c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(description.c_str());

    ImGui::SetCursorPos({ location.x + 170, location.y });

    if (is_int)
    {
        int value_as_int = static_cast<int>(value);
        value_changed = ImGui::SliderInt(("##" + name).c_str(), &value_as_int, static_cast<int>(range.min), static_cast<int>(range.max), "%.0f");
        value = static_cast<float>(value_as_int);
    }
    else
    {
        value_changed = ImGui::SliderFloat(("##" + name).c_str(), &value, range.min, range.max, "%.3f", 1.0f);
    }

    ImGui::PopItemWidth();

    if (!enabled)
    {
        ImGui::PopStyleColor(3);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
    return value_changed;
}

/**
    Constructor for filter_options, takes a name and a filter.
*/
filter_options::filter_options(const std::string name, rs2::process_interface& filter) :
    filter_name(name),
    filter(filter),
    is_enabled(true)
{
    const std::array<rs2_option, 3> possible_filter_options = {
        RS2_OPTION_FILTER_MAGNITUDE,
        RS2_OPTION_FILTER_SMOOTH_ALPHA,
        RS2_OPTION_FILTER_SMOOTH_DELTA
    };

    //Go over each filter option and create a slider for it
    for (rs2_option opt : possible_filter_options)
    {
        if (filter.supports(opt))
        {
            rs2::option_range range = filter.get_option_range(opt);
            supported_options[opt].range = range;
            supported_options[opt].value = range.def;
            supported_options[opt].is_int = is_all_integers(range);
            supported_options[opt].description = filter.get_option_description(opt);
            std::string opt_name = rs2_option_to_string(opt);
            supported_options[opt].name = name + "_" + opt_name;
            std::string prefix = "Filter ";
            supported_options[opt].label = name + " " + opt_name.substr(prefix.length());
        }
    }
}

filter_options::filter_options(filter_options&& other) :
    filter_name(std::move(other.filter_name)),
    filter(std::move(other.filter)),
    supported_options(std::move(other.supported_options)),
    is_enabled(other.is_enabled.load())
{

}

bool filter_options::is_all_integers(const rs2::option_range& range)
{
    const auto is_integer = [](float f)
    {
        return (fabs(fmod(f, 1)) < std::numeric_limits<float>::min());
    };

    return is_integer(range.min) && is_integer(range.max) &&
        is_integer(range.def) && is_integer(range.step);
}