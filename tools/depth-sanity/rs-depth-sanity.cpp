// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <sstream>
#include <iostream>
#include <memory>
#include <string>
#include <cmath>



using namespace rs2;
using namespace std;


vector<int> resolutions_by_fps_rewrite_it_as_lambda(int width,int height)
{

}


std::string error_message{""};
struct metrics
{
    double avg_dist;
    double std;
    double _percentage_of_non_null_pixels;
};

metrics analyze_depth_image(const rs2::video_frame& frame, float units)
{
    auto pixels = (const uint16_t*)frame.get_data();
    double sum_of_all_distances = 0;
    double mean_of_all_distances;
    //vector<double> all_squared_differences;
    double sum_of_all_squared_differences = 0;
    double standard_deviation;
    double number_of_non_null_pixels = 0;
    double percentage_of_non_null_pixels = 0;
    const auto w = frame.get_width();
    const auto h = frame.get_height();
    long long int num_of_examined_pixels = w*h;



    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            //std::cout << "Accessing index " << (y*w + x) << std::endl;
            auto depth_raw = pixels[y*w + x];

            if (depth_raw)
            {
                // units is float
                auto distance = depth_raw * units;
                sum_of_all_distances += distance;
                ++number_of_non_null_pixels;
            }


            //std::cout << distance << std::endl;
        }


    mean_of_all_distances = sum_of_all_distances / number_of_non_null_pixels;
    percentage_of_non_null_pixels = (number_of_non_null_pixels/ num_of_examined_pixels)*100;

    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            //std::cout << "Accessing index " << (y*w + x) << std::endl;
            auto depth_raw = pixels[y*w + x];

            if (depth_raw)
            {
                // units is float
                auto distance = depth_raw * units;
                sum_of_all_squared_differences+= std::pow((distance - mean_of_all_distances),2);
            }
        }

    standard_deviation = std::sqrt(sum_of_all_squared_differences/num_of_examined_pixels);
    metrics result{};
    result.avg_dist = mean_of_all_distances;
    result.std = standard_deviation;
    result._percentage_of_non_null_pixels = percentage_of_non_null_pixels;
    return result;
}

int main(int argc, char * argv[])
{
    context ctx;
    
    util::device_hub hub(ctx);

    auto finished = false;
    GLFWwindow* win;

    map<pair<int,int>,vector<int>> supported_fps_by_resolution;

    while (!finished)
    {
        try
        {
            std::set<pair<int,int>> resolutions;
            std::vector<std::string> resolutions_strs;
            std::vector<const char*> resolutions_chars;

            int default_width,default_height;

            auto dev = hub.wait_for_device();
            auto dpt = dev.first<depth_sensor>();



            auto modes = dpt.get_stream_modes();
            for (auto&& profile : dpt.get_stream_modes())
            {
                if (profile.stream == RS2_STREAM_DEPTH &&
                    profile.format == RS2_FORMAT_Z16)
                {
                    resolutions.insert(make_pair(profile.width,profile.height));
                    supported_fps_by_resolution[make_pair(profile.width,profile.height)].push_back(profile.fps);
                }
            }


            std::vector<const char*> presets_labels;
            std::vector<float> presets_numbers;
            /**
            * read option value from the device
            * \param[in] option   option id to be queried
            * \return float value of the option
            */
            //auto p = dpt.get_option(RS2_OPTION_ADVANCED_MODE_PRESET);
            //.set_option(RS2_OPTION_ADVANCED_MODE_PRESET, p);
            //auto text = dpt.get_option_value_description(RS2_OPTION_ADVANCED_MODE_PRESET, 1.f);
            auto range = dpt.get_option_range(RS2_OPTION_ADVANCED_MODE_PRESET);
            auto index_of_selected_preset = 0, counter = 0;
            auto units = dpt.get_depth_scale();

            for (auto i = range.min; i <= range.max; i += range.step, counter++)
            {
                 //if (range.step < 0.001f) return false;
                // what if (endpoint.get_option_value_description(RS2_OPTION_ADVANCED_MODE_PRESET, i) == nullptr)?

                /**
                   * get option value description (in case specific option value hold special meaning)
                   * \param[in] option     option id to be checked
                   * \param[in] value      value of the option
                   * \return human-readable (const char*) description of a specific value of an option or null if no special meaning
                   */
                presets_labels.push_back(dpt.get_option_value_description(RS2_OPTION_ADVANCED_MODE_PRESET, i));
                presets_numbers.push_back(i);
            }


            std::vector<pair<int,int>> resolutions_vec(resolutions.begin(), resolutions.end());



            default_width = resolutions_vec.at((resolutions_vec.size()-1)).first;
            default_height = resolutions_vec.at((resolutions_vec.size()-1)).second;
            int index_of_selected_resolution = (resolutions.size()-1);



            for (auto&& resolution : resolutions_vec)
            {
                resolutions_strs.push_back(rs2::to_string() << resolution.first << "x" << resolution.second);
            }

            for (auto&& resolution_as_string : resolutions_strs)
            {
                resolutions_chars.push_back(resolution_as_string.c_str());
            }



            // Configure depth stream to run at 30 frames per second
            util::config config;
            config.enable_stream(RS2_STREAM_DEPTH, default_width, default_height, 30, RS2_FORMAT_Z16);
            //config.enable_stream(RS2_STREAM_DEPTH, 640, 360, 30, RS2_FORMAT_Z16);
            auto stream = config.open(dev);

            syncer_processing_block syncer;
            stream.start(syncer);

            // black.start(syncer);
            frame_queue queue;
            syncer.start(queue);
            texture_buffer buffers[RS2_STREAM_COUNT];
            buffers[RS2_STREAM_DEPTH].equalize = false;
            buffers[RS2_STREAM_DEPTH].min_depth = 0.2 / dpt.get_depth_scale();
            buffers[RS2_STREAM_DEPTH].max_depth = 1.5 / dpt.get_depth_scale();

            // Open a GLFW window
            glfwInit();
            ostringstream ss;
            ss << "Depth Sanity (" << dev.get_info(RS2_CAMERA_INFO_NAME) << ")";

            //dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)




            win = glfwCreateWindow(1280, 720, ss.str().c_str(), nullptr, nullptr);
            glfwMakeContextCurrent(win);


            ImGui_ImplGlfw_Init(win, true);


            metrics latest_stat{};
            latest_stat.avg_dist = 0.0;
            latest_stat.std = 0.0;

            while (hub.is_connected(dev) && !glfwWindowShouldClose(win))
            {
                
                int w, h;
                glfwGetFramebufferSize(win, &w, &h);

                auto index = 0;
                auto frames = queue.wait_for_frames();

                for (auto&& frame : frames)
                {
                    auto stream_type = frame.get_stream_type();

                    if (stream_type == RS2_STREAM_DEPTH)
                    {
                        latest_stat = analyze_depth_image(frame, units);
                        cout << "Average distance is : " << latest_stat.avg_dist << endl;
                        cout << "Standard_deviation is : " << latest_stat.std << endl;
                        cout << "percentage_of_non_null_pixels is : " << latest_stat._percentage_of_non_null_pixels << endl;
                    }

                    buffers[stream_type].upload(frame);
                }

                // Wait for new images
                glfwPollEvents();
                ImGui_ImplGlfw_NewFrame();

                // Clear the framebuffer
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                // Draw the images
                glPushMatrix();
                glfwGetWindowSize(win, &w, &h);
                glOrtho(0, w, h, 0, -1, +1);

                for (auto&& frame : frames)
                {
                    auto stream_type = frame.get_stream_type();
                    buffers[stream_type].show({ 0, 0, (float)w, (float)h }, 1);
                }


                // Draw GUI:
                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.5f });
                //ImGui::SetNextWindowPos({ 10, 10 });
                ImGui::SetNextWindowPos({ w/100, h/100 });
                ImGui::SetNextWindowSize({ w/10.f, h/100.f });

//                ImGui::SetNextWindowPos({ 410, 360 });
//                ImGui::SetNextWindowSize({ 400.f, 350.f });
                ImGui::Begin("Stream Selector", nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

                rs2_error* e = nullptr;

                //ImFont* fnt;
                // Base font scale, multiplied by the per-window font scale which you can adjust with SetFontScale()
                //fnt->Scale=3.f;
                //ImGui::PushFont(fnt);

                ImGui::SetWindowFontScale(w/1000);


                ImGui::Text("SDK version: %s", api_version_to_string(rs2_get_api_version(&e)).c_str());
                //rs2_camera_info_to_string(rs2_camera_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)));
                ImGui::Text("Firmware: %s", dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION));
                ImGui::Text("Resolution:"); ImGui::SameLine();
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("Resolution", &index_of_selected_resolution, resolutions_chars.data(), resolutions_chars.size()))
                {
                    auto selected_resolution = resolutions_vec[index_of_selected_resolution];

                    stream.stop();
                    stream.close();
                    util::config config;

                    auto fps = supported_fps_by_resolution[selected_resolution].front();
                    // TODO: if you can find 30, use it
                    // else use whatever
                    config.enable_stream(RS2_STREAM_DEPTH, selected_resolution.first,
                                         selected_resolution.second, fps, RS2_FORMAT_Z16);
                    stream = config.open(dev);
                    stream.start(syncer);
                }




                ImGui::Text("Preset:"); ImGui::SameLine();
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("Preset", &index_of_selected_preset, presets_labels.data(), presets_labels.size()))
                {
                    /*
                     * another way to get the number of preset :
                     *  if (ImGui::Combo(id.c_str(), &index_of_selected_preset, labels.data(),
                        static_cast<int>(labels.size())))
                        {
                            value = range.min + range.step * index_of_selected_preset;
                            endpoint.set_option(opt, value);
                     */
                    auto selected_preset = presets_numbers[index_of_selected_preset];
                    /*
                     * the second parameter < float selected_preset> is an preset that should be sat
                     */

                    try
                    {
                        dpt.set_option(RS2_OPTION_ADVANCED_MODE_PRESET, selected_preset);
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }


                }

                ImGui::PopItemWidth();


                if (error_message != "")
                {
                    ImGui::OpenPopup("Oops, something went wrong!");
                }
                if (ImGui::BeginPopupModal("Oops, something went wrong!", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("RealSense error calling:");
                    ImGui::InputTextMultiline("error", const_cast<char*>(error_message.c_str()),
                        error_message.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll);
                    ImGui::Separator();

                    if (ImGui::Button("OK", ImVec2(120, 0)))
                    {
                        error_message = "";
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

               ImGui::End();
               ImGui::PopStyleColor();


               ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.5f });
               ImGui::SetNextWindowPos({ 0.85*w, 0.85*h });
               ImGui::SetNextWindowSize({ (.1*w), (.1*h)});

//               ImGui::SetNextWindowPos({ 1530, 930 });
//               ImGui::SetNextWindowSize({ 300.f, 370.f });

               ImGui::Begin("latest_stat", nullptr,
                                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
               ImGui::SetWindowFontScale(w/1000);

               ImGui::Text("Average: %.3f", latest_stat.avg_dist);
               ImGui::Text("STD: %.3f", latest_stat.std);
               ImGui::Text("Non null pixels: %.3f", latest_stat._percentage_of_non_null_pixels);
               ImGui::End();
               ImGui::PopStyleColor();


                ImGui::Render();
                glPopMatrix();
                glfwSwapBuffers(win);
            }


            if (glfwWindowShouldClose(win))
                finished = true;


            
        }
        catch (const error & e)
        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
        }
        catch (const exception & e)
        {
            cerr << e.what() << endl;
        }

        ImGui_ImplGlfw_Shutdown();
        glfwDestroyWindow(win);
        glfwTerminate();
    }
    return EXIT_SUCCESS;
}

