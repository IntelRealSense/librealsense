// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "model-views.h"
#include "ux-window.h"

#include <cstdarg>
#include <thread>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <array>
#include <mutex>
#include <set>

#include <imgui_internal.h>

// We use NOC file helper function for cross-platform file dialogs
#include <noc_file_dialog.h>

using namespace rs2;
using namespace rs400;

void add_playback_device(context& ctx, std::vector<device_model>& device_models, std::string& error_message, viewer_model& viewer_model, const std::string& file)
{
    bool was_loaded = false;
    bool failed = false;
    try
    {
        auto dev = ctx.load_device(file);
        was_loaded = true;
        device_models.emplace_back(dev, error_message, viewer_model);
        if (auto p = dev.as<playback>())
        {
            auto filename = p.file_name();
            p.set_status_changed_callback([&device_models, filename](rs2_playback_status status)
            {
                if (status == RS2_PLAYBACK_STATUS_STOPPED)
                {
                    auto it = std::find_if(device_models.begin(), device_models.end(),
                        [&](const device_model& dm) {
                        if (auto p = dm.dev.as<playback>())
                            return p.file_name() == filename;
                        return false;
                    });
                    if (it != device_models.end())
                    {
                        auto subs = it->subdevices;
                        if (it->_playback_repeat)
                        {
                            //Calling from different since playback callback is from reading thread
                            std::thread{ [subs]()
                            {
                                for (auto&& sub : subs)
                                {
                                    if (sub->streaming)
                                    {
                                        auto profiles = sub->get_selected_profiles();
                                        sub->play(profiles);
                                    }
                                }
                            } }.detach();
                        }
                        else
                        {
                            for (auto&& sub : subs)
                            {
                                if (sub->streaming)
                                {
                                    sub->stop();
                                }
                            }
                        }
                    }
                }
            });
        }
    }
    catch (const error& e)
    {
        error_message = to_string() << "Failed to load file " << file << ". Reason: " << error_to_string(e);
        failed = true;
    }
    catch (const std::exception& e)
    {
        error_message = to_string() << "Failed to load file " << file << ". Reason: " << e.what();
        failed = true;
    }
    if (failed && was_loaded)
    {
        try { ctx.unload_device(file); } catch (...){ }
    }
}

// This function is called every frame
// If between the frames there was an asyncronous connect/disconnect event
// the function will pick up on this and add the device to the viewer
void refresh_devices(std::mutex& m,
                     context& ctx,
                     bool& refresh_device_list,
                     device_list& list,
                     std::vector<std::pair<std::string, std::string>>& device_names,
                     std::vector<device_model>& device_models,
                     viewer_model& viewer_model,
                     std::vector<device>& devs,
                     std::string& error_message)
{
    std::lock_guard<std::mutex> lock(m);

    if (refresh_device_list)
    {
        refresh_device_list = false;

        try
        {
            auto prev_size = list.size();
            list = ctx.query_devices();

            device_names = get_devices_names(list);

            if (device_models.size() == 0 && list.size() > 0 && prev_size == 0)
            {
                auto dev = [&](){
                    for (size_t i = 0; i < list.size(); i++)
                    {
                        if (list[i].supports(RS2_CAMERA_INFO_NAME) &&
                            std::string(list[i].get_info(RS2_CAMERA_INFO_NAME)) != "Platform Camera")
                            return list[i];
                    }
                    return device();
                }();

                if (dev)
                {
                    device_models.emplace_back(dev, error_message, viewer_model);
                    viewer_model.not_model.add_log(to_string() << device_models.rbegin()->dev.get_info(RS2_CAMERA_INFO_NAME) << " was selected as a default device");
                }
            }

            devs.clear();
            for (auto&& sub : device_models)
            {
                devs.push_back(sub.dev);
                for (auto&& s : sub.dev.query_sensors())
                {
                    s.set_notifications_callback([&](const notification& n)
                    {
                        viewer_model.not_model.add_notification({ n.get_description(), n.get_timestamp(), n.get_severity(), n.get_category() });
                    });
                }
            }


            device_model* device_to_remove = nullptr;
            while(true)
            {
                for (auto&& dev_model : device_models)
                {
                    bool still_around = false;
                    for (auto&& dev : devs)
                        if (get_device_name(dev_model.dev) == get_device_name(dev))
                            still_around = true;
                    if (!still_around) {
                        for (auto&& s : dev_model.subdevices)
                            s->streaming = false;
                        device_to_remove = &dev_model;
                    }
                }
                if (device_to_remove)
                {
                    device_models.erase(std::find_if(begin(device_models), end(device_models),
                        [&](const device_model& other) { return get_device_name(other.dev) == get_device_name(device_to_remove->dev); }));
                    device_to_remove = nullptr;
                }
                else break;
            }
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
}

int main(int argv, const char** argc) try
{
    ux_window window("Intel RealSense Viewer");

    // Create RealSense Context
    context ctx;
    auto refresh_device_list = true;

    std::vector<std::pair<std::string, std::string>> device_names;

    std::string error_message{ "" };
    std::string label{ "" };

    std::vector<device_model> device_models;
    device_model* device_to_remove = nullptr;

    viewer_model viewer_model;
    device_list list;

    std::vector<device> devs;
    std::mutex m;

    periodic_timer update_readonly_options_timer(std::chrono::seconds(6));

    window.on_file_drop = [&](std::string filename)
    {
        std::string error_message{};
        add_playback_device(ctx, device_models, error_message, viewer_model, filename);
        if (!error_message.empty())
        {
            viewer_model.not_model.add_notification({ error_message,
                0, RS2_LOG_SEVERITY_ERROR, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
        }
    };

    ctx.set_devices_changed_callback([&](event_information& info)
    {
        std::lock_guard<std::mutex> lock(m);

        for (auto dev : devs)
        {
            if (info.was_removed(dev))
            {
                viewer_model.not_model.add_notification({ get_device_name(dev).first + " Disconnected\n",
                    0, RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
        }

        try
        {
            for (auto dev : info.get_new_devices())
            {
                viewer_model.not_model.add_notification({ get_device_name(dev).first + " Connected\n",
                    0, RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
        }
        catch (...)
        {

        }
        refresh_device_list = true;
    });

    for (int i = 1; i < argv; i++)
    {
        try
        {
            const char* arg = argc[i];
            std::ifstream file(arg);
            if (!file.good())
                continue;

            add_playback_device(ctx, device_models, error_message, viewer_model, arg);
        }
        catch (const rs2::error& e)
        {
            error_message = error_to_string(e);
        }
        catch (const std::exception& e)
        {
            error_message = e.what();
        }
    }

    window.on_load = [&]()
    {
        refresh_devices(m, ctx, refresh_device_list, list, device_names, device_models, viewer_model, devs, error_message);
    };

    // Closing the window
    while (window)
    {
        refresh_devices(m, ctx, refresh_device_list, list, device_names, device_models, viewer_model, devs, error_message);

        bool update_read_only_options = update_readonly_options_timer;

        auto output_height = viewer_model.get_output_height();

        rect viewer_rect = { viewer_model.panel_width,
                             viewer_model.panel_y, window.width() -
                             viewer_model.panel_width,
                             window.height() - viewer_model.panel_y - output_height };

        // Flags for pop-up window - no window resize, move or collaps
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings;

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ viewer_model.panel_width, viewer_model.panel_y });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Add Device Panel", nullptr, flags);

        ImGui::PushFont(window.get_large_font());
        ImGui::PushStyleColor(ImGuiCol_PopupBg, from_rgba(230, 230, 230, 255));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, from_rgba(0, 0xae, 0xff, 255));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, from_rgba(255, 255, 255, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        ImGui::SetNextWindowPos({ 0, viewer_model.panel_y });

        if (ImGui::Button(u8"Add Source\t\t\t\t\t\t\t\t\t\t\t\t\uf0d7", { viewer_model.panel_width - 1, viewer_model.panel_y }))
            ImGui::OpenPopup("select");

        auto new_devices_count = device_names.size() + 1;
        for (auto&& dev_model : device_models)
            if (list.contains(dev_model.dev) || dev_model.dev.is<playback>())
                new_devices_count--;

        ImGui::PushFont(window.get_font());
        ImGui::SetNextWindowSize({ viewer_model.panel_width, 20.f * new_devices_count + 8 });
        if (ImGui::BeginPopup("select"))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, dark_grey);
            ImGui::Columns(2, "DevicesList", false);
            for (size_t i = 0; i < device_names.size(); i++)
            {
                bool skip = false;
                for (auto&& dev_model : device_models)
                    if (get_device_name(dev_model.dev) == device_names[i]) skip = true;
                if (skip) continue;

                if (ImGui::Selectable(device_names[i].first.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)/* || switch_to_newly_loaded_device*/)
                {
                    try
                    {
                        auto dev = list[i];
                        device_models.emplace_back(dev, error_message, viewer_model);
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

                if (ImGui::IsItemHovered())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(255, 255, 255, 255));
                    ImGui::NextColumn();
                    ImGui::Text("S/N: %s", device_names[i].second.c_str());
                    ImGui::NextColumn();
                    ImGui::PopStyleColor();
                }
                else
                {
                    ImGui::NextColumn();
                    ImGui::Text("S/N: %s", device_names[i].second.c_str());
                    ImGui::NextColumn();
                }

            }

            if (new_devices_count > 1) ImGui::Separator();

            if (ImGui::Selectable("Load Recorded Sequence", false, ImGuiSelectableFlags_SpanAllColumns))
            {
                if (auto ret = file_dialog_open(open_file,"ROS-bag\0*.bag\0", NULL, NULL))
                {
                    add_playback_device(ctx, device_models, error_message, viewer_model, ret);
                }
            }
            ImGui::NextColumn();
            ImGui::Text("%s","");
            ImGui::NextColumn();

            ImGui::PopStyleColor();
            ImGui::EndPopup();
        }
        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::End();
        ImGui::PopStyleVar();


        viewer_model.show_top_bar(window, viewer_rect);

        viewer_model.show_event_log(window.get_font(), viewer_model.panel_width,
            window.height() - (viewer_model.is_output_collapsed ? viewer_model.default_log_h : 20),
            window.width() - viewer_model.panel_width, viewer_model.default_log_h);

        // Set window position and size
        ImGui::SetNextWindowPos({ 0, viewer_model.panel_y });
        ImGui::SetNextWindowSize({ viewer_model.panel_width, window.height() - viewer_model.panel_y });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, sensor_bg);

        // *********************
        // Creating window menus
        // *********************
        ImGui::Begin("Control Panel", nullptr, flags | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        if (device_models.size() > 0)
        {
            std::map<subdevice_model*, float> model_to_y;
            std::map<subdevice_model*, float> model_to_abs_y;
            auto windows_width = ImGui::GetContentRegionMax().x;

            for (auto&& dev_model : device_models)
            {
                dev_model.draw_controls(viewer_model.panel_width, viewer_model.panel_y,
                    window.get_font(), window.get_large_font(), window.get_mouse(),
                    error_message, device_to_remove, viewer_model, windows_width,
                    update_read_only_options,
                    model_to_y, model_to_abs_y);
            }

            if (device_to_remove)
            {
                if (auto p = device_to_remove->dev.as<playback>())
                {
                    ctx.unload_device(p.file_name());
                }

                device_models.erase(std::find_if(begin(device_models), end(device_models),
                    [&](const device_model& other) { return get_device_name(other.dev) == get_device_name(device_to_remove->dev); }));
                device_to_remove = nullptr;
            }

            ImGui::SetContentRegionWidth(windows_width);

            auto pos = ImGui::GetCursorScreenPos();
            auto h = ImGui::GetWindowHeight();
            if (h > pos.y - viewer_model.panel_y)
            {
                ImGui::GetWindowDrawList()->AddLine({ pos.x,pos.y }, { pos.x + viewer_model.panel_width,pos.y }, ImColor(from_rgba(0, 0, 0, 0xff)));
                ImRect bb(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + ImGui::GetContentRegionAvail().y));
                ImGui::GetWindowDrawList()->AddRectFilled(bb.GetTL(), bb.GetBR(), ImColor(dark_window_background));
            }

            for (auto&& dev_model : device_models)
            {
                bool stop_recording = false;
                for (auto&& sub : dev_model.subdevices)
                {
                    try
                    {
                        ImGui::SetCursorPos({ windows_width - 35, model_to_y[sub.get()] + 3 });
                        ImGui::PushFont(window.get_font());

                        ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                        if (!sub->streaming)
                        {
                            label = to_string() << u8"  \uf204\noff   ##" << dev_model.id << "," << sub->s.get_info(RS2_CAMERA_INFO_NAME);

                            ImGui::PushStyleColor(ImGuiCol_Text, redish);
                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

                            if (sub->is_selected_combination_supported())
                            {
                                if (ImGui::Button(label.c_str(), { 30,30 }))
                                {
                                    auto profiles = sub->get_selected_profiles();
                                    sub->play(profiles);

                                    for (auto&& profile : profiles)
                                    {
                                        viewer_model.streams[profile.unique_id()].begin_stream(sub, profile);
                                    }
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Start streaming data from this sensor");
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled(u8"  \uf204\noff   ");
                                if (std::any_of(sub->stream_enabled.begin(), sub->stream_enabled.end(), [](std::pair<int, bool> const& s) { return s.second; }))
                                {
                                    if (ImGui::IsItemHovered())
                                    {
                                        ImGui::SetTooltip("Selected configuration (FPS, Resolution) is not supported");
                                    }
                                }
                                else
                                {
                                    if (ImGui::IsItemHovered())
                                    {
                                        ImGui::SetTooltip("No stream selected");
                                    }
                                }

                            }
                        }
                        else
                        {
                            label = to_string() << u8"  \uf205\n    on##" << dev_model.id << "," << sub->s.get_info(RS2_CAMERA_INFO_NAME);
                            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);

                            if (ImGui::Button(label.c_str(), { 30,30 }))
                            {
                                sub->stop();

                                if (!std::any_of(dev_model.subdevices.begin(), dev_model.subdevices.end(),
                                    [](const std::shared_ptr<subdevice_model>& sm)
                                {
                                    return sm->streaming;
                                }))
                                {
                                    stop_recording = true;
                                }
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Stop streaming data from selected sub-device");
                            }
                        }
                    }
                    catch (const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch (const std::exception& e)
                    {
                        error_message = e.what();
                    }

                    ImGui::PopStyleColor(5);
                    ImGui::PopFont();
                }

                if (dev_model.is_recording && stop_recording)
                {
                    dev_model.stop_recording();
                    for (auto&& sub : dev_model.subdevices)
                    {
                        //TODO: Fix case where sensor X recorded stream 0, then stopped, and then started recording stream 1 (need 2 sensors for this to happen)
                        if (sub->is_selected_combination_supported())
                        {
                            auto profiles = sub->get_selected_profiles();
                            for (auto&& profile : profiles)
                            {
                                viewer_model.streams[profile.unique_id()].dev = sub;
                            }
                        }
                    }
                }
            }
        }
        else
        {
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            ImRect bb(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + ImGui::GetContentRegionAvail().y));
            ImGui::GetWindowDrawList()->AddRectFilled(bb.GetTL(), bb.GetBR(), ImColor(dark_window_background));

            viewer_model.show_no_device_overlay(window.get_large_font(), 50, viewer_model.panel_y + 50);
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Fetch frames from queues
        for (auto&& device_model : device_models)
            for (auto&& sub : device_model.subdevices)
            {
                sub->queues.foreach([&](frame_queue& queue)
                {
                    try
                    {
                        frame f;
                        if (queue.poll_for_frame(&f))
                        {
                            viewer_model.upload_frame(std::move(f));
                        }
                    }
                    catch (const error& ex)
                    {
                        error_message = error_to_string(ex);
                        sub->stop();
                    }
                    catch (const std::exception& ex)
                    {
                        error_message = ex.what();
                        sub->stop();
                    }
                });
            }

        viewer_model.gc_streams();

        window.begin_viewport();

        viewer_model.draw_viewport(viewer_rect, window, device_models.size(), error_message);

        viewer_model.not_model.draw(window.get_font(), window.width(), window.height());

        viewer_model.popup_if_error(window.get_font(), error_message);
    }

    // Stop calculating 3D model
    viewer_model.pc.stop();

    // Stop all subdevices
    for (auto&& device_model : device_models)
        for (auto&& sub : device_model.subdevices)
        {
            if (sub->streaming)
                sub->stop();
        }

    return EXIT_SUCCESS;
}
catch (const error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}