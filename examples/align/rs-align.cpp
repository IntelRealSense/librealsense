// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "../example.hpp"
#include <imgui.h>
#include "imgui_impl_glfw.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>

void render_slider(rect location, float& clipping_dist);
void remove_background(rs2::video_frame& color, const rs2::depth_frame& depth_frame, float depth_scale, float clipping_dist);
bool try_get_depth_scale(rs2::pipeline& p, float& scale);
int main(int argc, char * argv[]) try
{
    // Create and initialize GUI related objects
    window app(1280, 720, "CPP - Align Example"); // Simple window handling
    ImGui_ImplGlfw_Init(app, false);      // ImGui library intializition 
    rs2::colorizer c;                          // Helper to colorize depth images
    texture renderer;                     // Helper for renderig images

    // Create a context
    rs2::context ctx;
    // Using the context to create a rs2::align object. 
    // rs2::align allows you to perform aliment of depth frames to others
    rs2::align align(RS2_STREAM_COLOR);
    
    // Create a pipeline to easily configure and start the camera
    rs2::pipeline pipe;
    
    // By passing align to Pipeline::start, frames will be passed to align's handler
    pipe.start();
    
    // Each depth camera might have different units for depth pixels, so we get it here
    float depth_scale;
    if (!try_get_depth_scale(pipe, depth_scale))
    {
        std::cerr << "Device does not have a depth sensor" << std::endl;
        return EXIT_FAILURE;
    }

    // Define a variable for controlling the distance to clip
    float depth_clipping_distance = 1.f;

    while (app) // Application still alive?
    {
        // Using the align object, we block the application until a frameset is available
        rs2::frameset frameset;

        while (!frameset.get_color_frame() || !frameset.get_depth_frame())
        {
            frameset = pipe.wait_for_frames();
        }

        auto proccessed = align.proccess(frameset);

        // Trying to get both color and aligned depth frames
        rs2::video_frame color_frame = proccessed.get_color_frame();
        rs2::depth_frame aligned_depth_frame = proccessed.get_depth_frame();

        //If one of them is unavailable, continue iteration
        if (!aligned_depth_frame || !color_frame)
        {
            continue;
        }
        // Passing both frames to remove_background so it will "strip" the background
        // NOTE: in this example, we alter the buffer of the color frame, instead of copying it and altering the copy
        //       This behavior is not recommended in real application since the color frame could be used elsewhere
        remove_background(color_frame, aligned_depth_frame, depth_scale, depth_clipping_distance);

        // Taking dimensions of the window for rendering purposes
        float w = static_cast<float>(app.width());
        float h = static_cast<float>(app.height());

        // At this point, "color_frame" is an altered color frame, stripped form its background
        // Calculating the position to place the frame in the window
        rect altered_color_frame_rect{ 0, 0, w, h };
        altered_color_frame_rect = altered_color_frame_rect.adjust_ratio({ static_cast<float>(color_frame.get_width()),static_cast<float>(color_frame.get_height()) });
        
        // Render aligned color
        renderer.render(color_frame, altered_color_frame_rect);

        // The example also renders the depth frame, as a picture-in-picture
        // Calculating the position to place the depth frame in the window
        rect pip_stream{ 0, 0, w / 5, h / 5 };
        pip_stream = pip_stream.adjust_ratio({ static_cast<float>(aligned_depth_frame.get_width()),static_cast<float>(aligned_depth_frame.get_height()) });
        pip_stream.x = altered_color_frame_rect.x + altered_color_frame_rect.w - pip_stream.w - (std::max(w, h) / 25);
        pip_stream.y = altered_color_frame_rect.y + altered_color_frame_rect.h - pip_stream.h - (std::max(w, h) / 25);

        // Render depth (as picture in pipcture)
        renderer.upload(c(aligned_depth_frame));
        renderer.show(pip_stream);

        // Using ImGui library to provide a slide controller to select the depth clipping distance
        ImGui_ImplGlfw_NewFrame(1);
        render_slider({ 5.f, 0, w, h }, depth_clipping_distance);
        ImGui::Render();

    }
    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

bool try_get_depth_scale(rs2::pipeline& p, float& scale)
{
    // Get the underlying device from the pipeline
    rs2::device dev = p.get_device();

    // Go over the device's sensors
    for (rs2::sensor& sensor : dev.query_sensors())
    {
        // Check if the sensor if a depth sensor
        if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
        {
            scale = dpt.get_depth_scale();
            return true;
        }
    }
    return false;
}

void render_slider(rect location, float& clipping_dist)
{
    // Some trickery to display the control nicely 
    static const int flags = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove;
    const int pixels_to_buttom_of_stream_text = 25;
    const float slider_window_width = 30;

    ImGui::SetNextWindowPos({ location.x, location.y + pixels_to_buttom_of_stream_text });
    ImGui::SetNextWindowSize({ slider_window_width + 20, location.h - (pixels_to_buttom_of_stream_text * 2) });
    
    //Render the vertical slider
    ImGui::Begin("slider", nullptr, flags);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImColor(215.f / 255, 215.0f / 255, 215.0f / 255));
    auto slider_size = ImVec2(slider_window_width / 2, location.h - (pixels_to_buttom_of_stream_text * 2) - 20);
    ImGui::VSliderFloat("", slider_size, &clipping_dist, 0.0f, 6.0f, "", 1.0f, true);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Depth Clipping Distance: %.3f", clipping_dist);
    ImGui::PopStyleColor(3);

    //Display bars next to slider
    float bars_dist = (slider_size.y / 6.0f);
    for (int i = 0; i <= 6; i++)
    {
        ImGui::SetCursorPos({ slider_size.x, i * bars_dist });
        std::string bar_text = "- " + std::to_string(6-i) + "m";
        ImGui::Text("%s", bar_text.c_str());
    }
    ImGui::End();
}

void remove_background(rs2::video_frame& other_frame, const rs2::depth_frame& depth_frame, float depth_scale, float clipping_dist)
{
    const uint16_t* p_depth_frame = reinterpret_cast<const uint16_t*>(depth_frame.get_data());
    uint8_t* p_other_frame = reinterpret_cast<uint8_t*>(const_cast<void*>(other_frame.get_data()));

    int width = other_frame.get_width();
    int height = other_frame.get_height();
    int other_bpp = other_frame.get_bytes_per_pixel();

    #pragma omp parallel for schedule(dynamic) //Using OpenMP to try to parallelise the loop
    for (int y = 0; y < height; y++)
    {
        auto depth_pixel_index = y * width;
        for (int x = 0; x < width; x++, ++depth_pixel_index)
        {
            // Get the depth value of the current pixel
            auto pixels_distance = depth_scale * p_depth_frame[depth_pixel_index];

            // Check if the depth value is invalid (<=0) or greater than the threashold
            if (pixels_distance <= 0.f || pixels_distance > clipping_dist)
            {
                // Calculate the offset in other frame's buffer to current pixel
                auto offset = depth_pixel_index * other_bpp;

                // Set pixel to "background" color (0x999999)
                std::memset(&p_other_frame[offset], 0x99, other_bpp);
            }
        }
    }
}
