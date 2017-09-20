// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/hpp/rs_sensor.hpp>
//#include <librealsense2/rsutil.h>
#include <librealsense2/rs.hpp>
#include <sstream>
#include <memory>
#include <string>
#include <cmath>
#include <thread>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "realsense-ui-advanced-mode.h"
#include "model-views.h"
#include "depth_metrics.h"
#include "depth_quality_viewer.h"
#include "depth_quality_model.h"

using namespace rs2;
using namespace rs400;
using namespace rs2::depth_profiler;

std::string error_message{ "" };
color_map my_map({ { 255, 255, 255 },{ 0, 0, 0 } });

struct option_data
{
    bool supported;
    union { bool b; float f; } value;
    option_range range;

    option_data(depth_sensor s, rs2_option o, bool isBool = false) : supported(s.supports(o)), range(supported ? s.get_option_range(o) : option_range{ 0, 1, 0, 1 }) {
        if (isBool) value.b = (supported ? (bool)s.get_option(o) : 0);
        else value.f = (supported ? s.get_option(o) : 0);
    }
};

std::vector<std::string> get_device_info(const device& dev, bool include_location = true)
{
    std::vector<std::string> res;
    for (auto i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
    {
        auto info = static_cast<rs2_camera_info>(i);

        // When camera is being reset, either because of "hardware reset"
        // or because of switch into advanced mode,
        // we don't want to capture the info that is about to change
        if ((info == RS2_CAMERA_INFO_PHYSICAL_PORT ||
            info == RS2_CAMERA_INFO_ADVANCED_MODE)
            && !include_location) continue;

        if (dev.supports(info))
        {
            auto value = dev.get_info(info);
            res.push_back(value);
        }
    }
    return res;
}

int main(int argc, char * argv[])
{

    depth_profiler_model app;

    rs2::pipeline pipe;

    pipe.enable_stream(RS2_STREAM_DEPTH, 0, 0, 0, RS2_FORMAT_Z16, 30);
    //pipe.enable_stream(RS2_STREAM_INFRARED, 1, 0, 0, RS2_FORMAT_Y8, 30);
    //pipe.enable_stream(RS2_STREAM_COLOR, 0, 0, 0, RS2_FORMAT_RGB8, 30);

    // Wait till a valid device is found
    pipe.start();

    // Select the depth camera to work with
    app.use_device(pipe.get_device());

    while (app)     // Update internal state and render UI
    {
        try
        {
            frameset frames;
            if (pipe.poll_for_frames(&frames))
            {
                app.upload(frames);

                rs2::frame depth = frames.get_depth_frame();
                if (depth)
                    app.enqueue_for_processing(depth);
            }

            // Yield the CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }


    //    try
    //    {
    //        std::set<std::pair<int, int>> resolutions;
    //        std::vector<std::string> resolutions_strs;
    //        std::vector<const char*> resolutions_chars;
    //        int default_width{}, default_height{};
    //        // find the first device that provides depth data
    //        rs2::device dev{};
    //        std::string last_dev_found{};
    //        bool not_found{ true };

    //        do {
    //            dev = hub.wait_for_device();

    //            for (auto s : dev.query_sensors())
    //            {
    //                if (s.is<depth_sensor>())
    //                {
    //                    std::cout << "Depth Camera " << dev.get_info(RS2_CAMERA_INFO_NAME) << " acquired" << std::endl;
    //                    not_found = false;
    //                    break;
    //                }
    //            }

    //            if (not_found)
    //            {
    //                std::string dev_name(dev.get_info(RS2_CAMERA_INFO_NAME));
    //                if (last_dev_found != dev_name)
    //                {
    //                    last_dev_found = dev_name;
    //                    std::cout << "Device " << dev.get_info(RS2_CAMERA_INFO_NAME)
    //                        << " is not a supported depth camera, waiting for Intel Realsense device to connect" << std::endl;
    //                }
    //                std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //            }
    //        } while (not_found);

    //        std::cout << "device was found" << std::endl;
    //        auto dpt = dev.first<depth_sensor>();

    //        option_data ae(dpt, RS2_OPTION_ENABLE_AUTO_EXPOSURE, true);
    //        option_data exposure(dpt, RS2_OPTION_EXPOSURE);
    //        option_data emitter(dpt, RS2_OPTION_EMITTER_ENABLED, true);
    //        option_data laser(dpt, RS2_OPTION_LASER_POWER);

    //        auto profiles = dpt.get_stream_profiles();
    //        for (auto&& profile : profiles)
    //        {
    //            if (profile.stream_type() == RS2_STREAM_DEPTH &&
    //                profile.format() == RS2_FORMAT_Z16)
    //            {
    //                auto video_profile = profile.as<video_stream_profile>();
    //                resolutions.insert(std::make_pair(video_profile.width(), video_profile.height()));
    //                supported_fps_by_resolution[std::make_pair(video_profile.width(), video_profile.height())].push_back(profile.fps());
    //            }
    //        }

    //        std::vector<const char*> presets_labels;
    //        std::vector<float> presets_numbers;

    //        auto range = dpt.get_option_range(RS2_OPTION_VISUAL_PRESET);
    //        auto index_of_selected_preset = 0, counter = 0;

    //        for (auto i = range.min; i <= range.max; i += range.step, counter++)
    //        {
    //            presets_labels.push_back(dpt.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i));
    //            presets_numbers.push_back(i);
    //        }

    //        std::vector<std::pair<int, int>> resolutions_vec(resolutions.begin(), resolutions.end());

    //        default_width = resolutions_vec.at((resolutions_vec.size() - 1)).first;
    //        default_height = resolutions_vec.at((resolutions_vec.size() - 1)).second;
    //        int index_of_selected_resolution = (resolutions.size() - 1);

    //        for (auto&& resolution : resolutions_vec)
    //        {
    //            resolutions_strs.push_back(rs2::to_string() << resolution.first << "x" << resolution.second);
    //        }

    //        for (auto&& resolution_as_string : resolutions_strs)
    //        {
    //            resolutions_chars.push_back(resolution_as_string.c_str());
    //        }

    //        std::cout << "Depth scale is " << dpt.get_depth_scale() << std::endl;

    //        // Configure depth stream to run at 30 frames per second
    //        frame_queue calc_queue(1);
    //        frame_queue display_queue(1);

    //        rs2::pipeline pipe;
    //        pipe.enable_stream(RS2_STREAM_DEPTH, 0, default_width, default_height, RS2_FORMAT_Z16, 30);

    //        pipe.start();
    ////        pipe.start([&](rs2::frame f)
    ////        {
    //            //
    ////            calc_queue.enqueue(f);
    ////            display_queue.enqueue(f);
    //            //auto stream_type = f.get_profile().stream_type();
    //            //std::cout << " enque " << rs2_stream_to_string(stream_type) << std::endl;
    //            //auto format = rs2_format_to_string(f.get_profile().format());
    //            //auto size = f.get_profile().size();
    //            //auto size = f.get_profile().;

    //            //std::cout << " enque format " << format << " size : " << size << std::endl;
    ////        });

    //        auto pf = pipe.get_active_streams();
    //        rs2_intrinsics current_frame_intrinsics = pf.front().as<video_stream_profile>().get_intrinsics();
    //        auto dev1 = pipe.get_device();
    //        auto dpt1 = dev1.first<depth_sensor>();

    //        auto units = dpt1.get_depth_scale();
    //        std::cout << "Depth scale is " << dpt1.get_depth_scale() << std::endl;
    //        texture_buffer buffers[RS2_STREAM_COUNT];

    //        float roi_x_begin = default_width * (1.f / 3.f), roi_y_begin = default_height * (1.f / 3.f), roi_x_end = default_width * (2.f / 3.f), roi_y_end = default_height * (2.f / 3.f);
    //        region_of_interest roi{ roi_x_begin, roi_y_begin, roi_x_end, roi_y_end };
    //        img_metrics latest_stat{ 1280, 720, roi, { 0, 0, 0 }, { 0, 0, 0 }, 0 };
    //        std::mutex m;

    //        std::thread t([&m, &finished, &calc_queue, &units, &current_frame_intrinsics, &latest_stat, &roi]() {
    //            while (!finished)
    //            {
    //            
    //                auto f = calc_queue.wait_for_frame();

    //                frameset frame_set;
    //                frame_set = f.as<frameset>();
    //                auto ggg = frame_set.get_depth_frame();

    //                auto format = rs2_format_to_string(frame_set.get_profile().format());
    //                auto size = frame_set.get_profile().size();

    //                std::cout << " wait_for_frame format " << format << " size : " << size << std::endl;
    //                auto stream_type = frame_set.get_profile().stream_type();
    //                if (stream_type == RS2_STREAM_DEPTH)
    //                {
    //                    auto stats = analyze_depth_image(f, units, &current_frame_intrinsics, roi);

    //                    std::lock_guard<std::mutex> lock(m);
    //                    latest_stat = stats;
    //                    //cout << "Average distance is : " << latest_stat.avg_dist << endl;
    //                    //cout << "Standard_deviation is : " << latest_stat.std << endl;
    //                    //cout << "Percentage_of_non_null_pixels is : " << latest_stat.non_null_pct << endl;
    //                }
    //            }
    //        });

    //        while (hub.is_connected(dev) && !glfwWindowShouldClose(win))
    //        {

    //            int w, h;
    //            glfwGetFramebufferSize(win, &w, &h);

    //            auto index = 0;

    //            // Wait for new images
    //            glfwPollEvents();
    //            ImGui_ImplGlfw_NewFrame(1.f);

    //            // Clear the framebuffer
    //            glViewport(0, 0, w, h);
    //            glClear(GL_COLOR_BUFFER_BIT);

    //            // Draw the images
    //            glPushMatrix();
    //            glfwGetWindowSize(win, &w, &h);
    //            glOrtho(0, w, h, 0, -1, +1);

    //            auto f = display_queue.wait_for_frame();

    //            auto stream_type = f.get_profile().stream_type();

    //            if (stream_type == RS2_STREAM_DEPTH)
    //            {
    //                buffers[stream_type].upload(f);
    //            }

    //            buffers[RS2_STREAM_DEPTH].show({ 0, 0, (float)w, (float)h }, 1);

    //            img_metrics stats_copy;
    //            {
    //                std::lock_guard<std::mutex> lock(m);
    //                stats_copy = latest_stat;
    //            }

    //            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0,0,0,0 });
    //            ImGui::SetNextWindowPos({ 0, 0 });
    //            ImGui::SetNextWindowSize({ (float)w, (float)h });
    //            ImGui::Begin("global", nullptr,
    //                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);

    //            ImGuiIO& io = ImGui::GetIO();

    //            if (ImGui::IsWindowFocused() && ImGui::IsMouseClicked(0))
    //            {
    //                auto pos = ImGui::GetIO().MousePos;
    //                roi_x_begin = roi_x_end = pos.x;
    //                roi_y_begin = roi_y_end = pos.y;
    //            }

    //            if (ImGui::IsWindowFocused() && ImGui::IsMouseDragging())
    //            {
    //                roi_x_end += ImGui::GetIO().MouseDelta.x;
    //                roi_y_end += ImGui::GetIO().MouseDelta.y;
    //            }

    //            roi = { int(std::max(std::min(roi_x_begin, roi_x_end), 0.f)), int(std::max(std::min(roi_y_begin, roi_y_end), 0.f)), int(std::min(std::max(roi_x_begin, roi_x_end), float(stats_copy.width))), int(std::min(std::max(roi_y_begin, roi_y_end), float(stats_copy.height))) };

    //            visualize(stats_copy, w, h, use_rect_fitting);

    //            ImGui::End();
    //            ImGui::PopStyleColor();

    //            // Draw GUI:
    //            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.8f });

    //            //ImGui::SetNextWindowPos({ 10, 10 });
    //            ImGui::SetNextWindowPos({ margin, margin });
    //            ImGui::SetNextWindowSize({ 300, 200 });

    //            //                ImGui::SetNextWindowPos({ 410, 360 });
    //            //                ImGui::SetNextWindowSize({ 400.f, 350.f });
    //            ImGui::Begin("Stream Selector", nullptr,
    //                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);


    //            rs2_error* e = nullptr;

    //            //ImFont* fnt;
    //            // Base font scale, multiplied by the per-window font scale which you can adjust with SetFontScale()
    //            //fnt->Scale=3.f;
    //            //ImGui::PushFont(fnt);

    //            ImGui::Text("SDK version: %s", api_version_to_string(rs2_get_api_version(&e)).c_str());
    //            //rs2_camera_info_to_string(rs2_camera_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)));
    //            ImGui::Text("Firmware: %s", dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION));
    //            ImGui::Text("Resolution:"); ImGui::SameLine();
    //            ImGui::PushItemWidth(-1);
    //            if (ImGui::Combo("Resolution", &index_of_selected_resolution, resolutions_chars.data(), resolutions_chars.size()))
    //            {
    //                auto selected_resolution = resolutions_vec[index_of_selected_resolution];

    //                pipe.stop();

    //                auto fps = supported_fps_by_resolution[selected_resolution].front();
    //                // TODO: if you can find 30, use it
    //                // else use whatever
    //                pipe.enable_stream(RS2_STREAM_DEPTH, 0, selected_resolution.first,
    //                    selected_resolution.second, RS2_FORMAT_Z16, fps);
    //                //stream = config.open(dev);
    //                pipe.start();
    //                //pipe.start([&](rs2::frame f)
    //                //{
    //                //    calc_queue.enqueue(f);
    //                //    display_queue.enqueue(f);
    //                //});

    //            }
    //            ImGui::Text("Preset:"); ImGui::SameLine();
    //            ImGui::PushItemWidth(-1);
    //            if (ImGui::Combo("Preset", &index_of_selected_preset, presets_labels.data(), presets_labels.size()))
    //            {
    //                /*
    //                 * another way to get the number of preset :
    //                 *  if (ImGui::Combo(id.c_str(), &index_of_selected_preset, labels.data(),
    //                    static_cast<int>(labels.size())))
    //                    {
    //                        value = range.min + range.step * index_of_selected_preset;
    //                        endpoint.set_option(opt, value);
    //                 */
    //                auto selected_preset = presets_numbers[index_of_selected_preset];
    //                /*
    //                 * the second parameter < float selected_preset> is an preset that should be sat
    //                 */

    //                try
    //                {
    //                    dpt.set_option(RS2_OPTION_VISUAL_PRESET, selected_preset);
    //                }
    //                catch (const error& e)
    //                {
    //                    error_message = error_to_string(e);
    //                }
    //                catch (const std::exception& e)
    //                {
    //                    error_message = e.what();
    //                }
    //            }

    //            if (ae.supported && ImGui::Checkbox("Enable Auto Exposure", &ae.value.b))
    //            {
    //                try
    //                {
    //                    dpt.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, ae.value.b);
    //                }
    //                catch (const error& e)
    //                {
    //                    error_message = error_to_string(e);
    //                }
    //                catch (const std::exception& e)
    //                {
    //                    error_message = e.what();
    //                }
    //            }

    //            if (exposure.supported && ImGui::SliderFloat("Exposure", &exposure.value.f, exposure.range.min, exposure.range.max, "%.3f"))
    //            {
    //                try
    //                {
    //                    if (ae.supported) dpt.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, false);
    //                    dpt.set_option(RS2_OPTION_EXPOSURE, exposure.value.f);
    //                }
    //                catch (const error& e)
    //                {
    //                    error_message = error_to_string(e);
    //                }
    //                catch (const std::exception& e)
    //                {
    //                    error_message = e.what();
    //                }
    //            }

    //            if (emitter.supported && ImGui::Checkbox("Emitter Powered", &emitter.value.b))
    //            {
    //                try
    //                {
    //                    dpt.set_option(RS2_OPTION_EMITTER_ENABLED, emitter.value.b);
    //                }
    //                catch (const error& e)
    //                {
    //                    error_message = error_to_string(e);
    //                }
    //                catch (const std::exception& e)
    //                {
    //                    error_message = e.what();
    //                }
    //            }

    //            if (laser.supported && ImGui::SliderFloat("Laser Power", &laser.value.f, laser.range.min, laser.range.max, "%.3f"))
    //            {
    //                try
    //                {
    //                    if (emitter.value.b)
    //                        dpt.set_option(RS2_OPTION_LASER_POWER, laser.value.f);
    //                }
    //                catch (const error& e)
    //                {
    //                    error_message = error_to_string(e);
    //                }
    //                catch (const std::exception& e)
    //                {
    //                    error_message = e.what();
    //                }
    //            }

    //            ImGui::Checkbox("Use Plane-Fitting", &use_rect_fitting);

    //            ImGui::PopItemWidth();


    //            if (error_message != "")
    //            {
    //                ImGui::OpenPopup("Oops, something went wrong!");
    //            }
    //            if (ImGui::BeginPopupModal("Oops, something went wrong!", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    //            {
    //                ImGui::Text("RealSense error calling:");
    //                ImGui::InputTextMultiline("error", const_cast<char*>(error_message.c_str()),
    //                    error_message.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll);
    //                ImGui::Separator();

    //                if (ImGui::Button("OK", ImVec2(120, 0)))
    //                {
    //                    error_message = "";
    //                    ImGui::CloseCurrentPopup();
    //                }

    //                ImGui::EndPopup();
    //            }

    //            ImGui::End();
    //            ImGui::PopStyleColor();

    //            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.9f });

    //            ImGui::SetNextWindowPos({ w - 196 - margin, h - 336 - margin });
    //            ImGui::SetNextWindowSize({ 196, 336 });

    //            //               ImGui::SetNextWindowPos({ 0.85*w, 0.90*h });
    //            //               ImGui::SetNextWindowSize({ (.1*w), (.1*h)});

    //            //               ImGui::SetNextWindowPos({ 1530, 930 });
    //            //               ImGui::SetNextWindowSize({ 300.f, 370.f });

    //            ImGui::Begin("latest_stat", nullptr,
    //                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
    //            //ImGui::SetWindowFontScale(w/1000);

    //            metrics data = use_rect_fitting ? stats_copy.plane : stats_copy.depth;

    //            avg_plot.add_value(data.avg_dist);
    //            std_plot.add_value(data.std_dev);
    //            fill_plot.add_value(stats_copy.non_null_pct);
    //            dist_plot.add_value(data.distance);
    //            angle_plot.add_value(data.angle);
    //            out_plot.add_value(data.outlier_pct);

    //            avg_plot.plot();
    //            std_plot.plot();
    //            fill_plot.plot();
    //            dist_plot.plot();
    //            angle_plot.plot();
    //            out_plot.plot();

    //            ImGui::End();
    //            ImGui::PopStyleColor();

    //            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0.7f });

    //            ImGui::SetNextWindowPos({ float((w - 345) / 2), margin });
    //            ImGui::SetNextWindowSize({ 345, 28 });

    //            ImGui::Begin("##help", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    //            ImGui::Text("Click and drag to select the Region of Interest");

    //            ImGui::End();
    //            ImGui::PopStyleColor();

    //            ImGui::Render();
    //            glPopMatrix();
    //            glfwSwapBuffers(win);
    //        }

    //        if (glfwWindowShouldClose(win))
    //            finished = true;

    //        t.join();

    //    }
        catch (const error & e)
        {
            std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
        }
        catch (const std::exception & e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    pipe.stop();
    return EXIT_SUCCESS;
}
