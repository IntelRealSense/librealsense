// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.


#include <librealsense2/rs.hpp>
#ifdef NETWORK_DEVICE
#include <librealsense2-net/rs_net.hpp>
#endif
#include "viewer.h"
#include "os.h"
#include "ux-window.h"
#include "fw-update-helper.h"

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
#include <regex>

#include <imgui_internal.h>

#ifdef INTERNAL_FW
#include "common/fw/D4XX_FW_Image.h"
#include "common/fw/SR3XX_FW_Image.h"
#else
#define FW_D4XX_FW_IMAGE_VERSION ""
#define FW_SR3XX_FW_IMAGE_VERSION ""
#endif // INTERNAL_FW

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif
using namespace rs2;
using namespace rs400;

#define MIN_IP_SIZE 7 //TODO: Ester - update size when host name is supported

bool add_remote_device(context& ctx, std::string address) 
{
#ifdef NETWORK_DEVICE
    rs2::net_device dev(address);
    dev.add_to(ctx);
    return true; // NEtwork device exists
#else
    return false;
#endif
}

void add_playback_device(context& ctx, device_models_list& device_models, 
    std::string& error_message, viewer_model& viewer_model, const std::string& file)
{
    bool was_loaded = false;
    bool failed = false;
    try
    {
        auto dev = ctx.load_device(file);
        was_loaded = true;
        device_models.emplace_back(new device_model(dev, error_message, viewer_model)); //Will cause the new device to appear in the left panel
        if (auto p = dev.as<playback>())
        {
            auto filename = p.file_name();
            p.set_status_changed_callback([&viewer_model, &device_models, filename](rs2_playback_status status)
            {
                if (status == RS2_PLAYBACK_STATUS_STOPPED)
                {
                    auto it = std::find_if(device_models.begin(), device_models.end(),
                        [&](const std::unique_ptr<device_model>& dm) {
                        if (auto p = dm->dev.as<playback>())
                            return p.file_name() == filename;
                        return false;
                    });
                    if (it != device_models.end())
                    {
                        auto subs = (*it)->subdevices;
                        if ((*it)->_playback_repeat)
                        {
                            //Calling from different since playback callback is from reading thread
                            std::thread{ [subs, &viewer_model, it]()
                            {
                                if(!(*it)->dev_syncer)
                                    (*it)->dev_syncer = viewer_model.syncer->create_syncer();

                                for (auto&& sub : subs)
                                {
                                    if (sub->streaming)
                                    {
                                        auto profiles = sub->get_selected_profiles();

                                        sub->play(profiles, viewer_model, (*it)->dev_syncer);
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
                                    sub->stop(viewer_model);
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
        try { ctx.unload_device(file); }
        catch (...) {}
    }
}

// This function is called every frame
// If between the frames there was an asyncronous connect/disconnect event
// the function will pick up on this and add the device to the viewer
bool refresh_devices(std::mutex& m,
    context& ctx,
    device_changes& devices_connection_changes,
    std::vector<device>& current_connected_devices,
    std::vector<std::pair<std::string, std::string>>& device_names,
    device_models_list& device_models,
    viewer_model& viewer_model,
    std::string& error_message)
{
    event_information info({}, {});
    if (!devices_connection_changes.try_get_next_changes(info))
        return false;
    try
    {
        auto prev_size = current_connected_devices.size();

        //Remove disconnected
        auto dev_itr = begin(current_connected_devices);
        while (dev_itr != end(current_connected_devices))
        {
            auto dev = *dev_itr;
            if (info.was_removed(dev))
            {
                //Notify change
                viewer_model.not_model.add_notification({ get_device_name(dev).first + " Disconnected\n",
                    RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });

                //Remove from devices
                auto dev_model_itr = std::find_if(begin(device_models), end(device_models),
                    [&](const std::unique_ptr<device_model>& other) { return get_device_name(other->dev) == get_device_name(dev); });

                if (dev_model_itr != end(device_models))
                {
                    for (auto&& s : (*dev_model_itr)->subdevices)
                        s->streaming = false;

                    dev_model_itr->reset();
                    device_models.erase(dev_model_itr);

                    if (device_models.size() == 0)
                    {
                        viewer_model.ppf.depth_stream_active = false;

                        // Stopping post processing filter rendering thread in case of disconnection
                        viewer_model.ppf.stop();
                    }
                }
                auto dev_name_itr = std::find(begin(device_names), end(device_names), get_device_name(dev));
                if (dev_name_itr != end(device_names))
                    device_names.erase(dev_name_itr);

                dev_itr = current_connected_devices.erase(dev_itr);
                continue;
            }
            ++dev_itr;
        }

        //Add connected
        static bool initial_refresh = true;
        for (auto dev : info.get_new_devices())
        {
            auto dev_descriptor = get_device_name(dev);
            device_names.push_back(dev_descriptor);

            bool added = false;
            if (device_models.size() == 0 &&
                dev.supports(RS2_CAMERA_INFO_NAME) && std::string(dev.get_info(RS2_CAMERA_INFO_NAME)) != "Platform Camera" && std::string(dev.get_info(RS2_CAMERA_INFO_NAME)).find("IP Device") == std::string::npos)
            {
                device_models.emplace_back(new device_model(dev, error_message, viewer_model));
                viewer_model.not_model.add_log(to_string() << (*device_models.rbegin())->dev.get_info(RS2_CAMERA_INFO_NAME) << " was selected as a default device");
                added = true;
            }

            if (!initial_refresh)
            {
                if (added || dev.is<playback>())
                    viewer_model.not_model.add_notification({ dev_descriptor.first + " Connected\n",
                        RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
                else if (added || dev.supports(RS2_CAMERA_INFO_IP_ADDRESS))
                    viewer_model.not_model.add_notification({ dev_descriptor.first + " Connected\n",
                        RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
                else
                    viewer_model.not_model.add_notification({ dev_descriptor.first + " Connected\n",
                        RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR }, 
                        [&device_models, &viewer_model, &error_message, dev]{
                            auto device = dev;
                            device_models.emplace_back(
                                new device_model(device, error_message, viewer_model));
                        });
            }

            current_connected_devices.push_back(dev);
            for (auto&& s : dev.query_sensors())
            {
                s.set_notifications_callback([&, dev_descriptor](const notification& n)
                {
                    if (n.get_category() == RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT)
                    {
                        auto data = n.get_serialized_data();
                        if (!data.empty())
                        {
                            auto dev_model_itr = std::find_if(begin(device_models), end(device_models),
                                [&](const std::unique_ptr<device_model>& other) 
                                { return get_device_name(other->dev) == dev_descriptor; });

                            if (dev_model_itr == end(device_models))
                                return;

                            (*dev_model_itr)->handle_hardware_events(data);
                        }
                    }
                    viewer_model.not_model.add_notification({ n.get_description(), n.get_severity(), n.get_category() });
                });
            }

            
        }
        initial_refresh = false;
    }
    catch (const error& e)
    {
        error_message = error_to_string(e);
    }
    catch (const std::exception& e)
    {
        error_message = e.what();
    }
    catch (...)
    {
        error_message = "Unknown error";
    }
    return true;
}


int main(int argc, const char** argv) try
{
    rs2::log_to_console(RS2_LOG_SEVERITY_WARN);

    context ctx;
    ux_window window("Intel RealSense Viewer", ctx);

    // Create RealSense Context
    device_changes devices_connection_changes(ctx);
    std::vector<std::pair<std::string, std::string>> device_names;

    std::string error_message{ "" };
    std::string label{ "" };

    std::shared_ptr<device_models_list> device_models = std::make_shared<device_models_list>();
    device_model* device_to_remove = nullptr;
    bool is_ip_device_connected = false;
    std::string ip_address;

    viewer_model viewer_model(ctx);

    std::vector<device> connected_devs;
    std::mutex m;

    if (argc == 2)
    {
        try
        {
            is_ip_device_connected = add_remote_device(ctx, argv[1]);;
        }
        catch (std::runtime_error e)
        {
            error_message = e.what();
        }
    }

#ifdef BUILD_SHARED_LIBS
    // Configure the logger
    el::Configurations conf;
    conf.set( el::Level::Global, el::ConfigurationType::Format, "[%level] %msg" );
    conf.set( el::Level::Info, el::ConfigurationType::Format, "%msg" );
    conf.set( el::Level::Debug, el::ConfigurationType::Enabled, "false" );
    el::Loggers::reconfigureLogger( "default", conf );
    // Create a dispatch sink which will get any messages logged to EasyLogging, which will then
    // post the messages on the viewer's notification window.
    class viewer_model_dispatcher : public el::LogDispatchCallback
    {
    public:
        rs2::viewer_model * vm = nullptr;  // only the default ctor is available to us...!
    protected:
        void handle( const el::LogDispatchData* data ) noexcept override
        {
            // TODO align LRS and Easyloging severity levels. W/A for easylogging on Linux
            if (data->logMessage()->level() > el::Level::Debug)
            {
                vm->not_model.add_log(
                    data->logMessage()->logger()->logBuilder()->build(
                        data->logMessage(),
                        data->dispatchAction() == el::base::DispatchAction::NormalLog));
            }
        }
    };
    el::Helpers::installLogDispatchCallback< viewer_model_dispatcher >( "viewer_model_dispatcher" );
    auto dispatcher = el::Helpers::logDispatchCallback< viewer_model_dispatcher >( "viewer_model_dispatcher" );
    dispatcher->vm = &viewer_model;
    el::Helpers::uninstallLogDispatchCallback< el::base::DefaultLogDispatchCallback >( "DefaultLogDispatchCallback" );
#else
     rs2::log_to_callback( RS2_LOG_SEVERITY_INFO,
         [&]( rs2_log_severity severity, rs2::log_message const& msg )
         {
             viewer_model.not_model.add_log( msg.raw() );
         } );
#endif
    window.on_file_drop = [&](std::string filename)
    {
        
        add_playback_device(ctx, *device_models, error_message, viewer_model, filename);
        if (!error_message.empty())
        {
            viewer_model.not_model.add_notification({ error_message,
                RS2_LOG_SEVERITY_ERROR, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
        }
    };

    for (int i = 1; i < argc; i++)
    {
        try
        {
            const char* arg = argv[i];
            std::ifstream file(arg);
            if (!file.good())
                continue;

            add_playback_device(ctx, *device_models, error_message, viewer_model, arg);
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
        refresh_devices(m, ctx, devices_connection_changes, connected_devs,
            device_names, *device_models, viewer_model, error_message);
        return true;
    };


    // Closing the window
    while (window)
    {
        auto device_changed = refresh_devices(m, ctx, devices_connection_changes, connected_devs, 
            device_names, *device_models, viewer_model, error_message);

        auto output_height = viewer_model.get_output_height();

        rect viewer_rect = { viewer_model.panel_width,
                             viewer_model.panel_y, window.width() -
                             viewer_model.panel_width,
                             window.height() - viewer_model.panel_y - output_height };

        // Flags for pop-up window - no window resize, move or collaps
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

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

        std::string add_source_button_text = to_string() << " " << textual_icons::plus_circle << "  Add Source\t\t\t\t\t\t\t\t\t\t\t";
        if (ImGui::Button(add_source_button_text.c_str(), { viewer_model.panel_width - 1, viewer_model.panel_y }))
            ImGui::OpenPopup("select");

        auto new_devices_count = device_names.size() + 1;

        for (auto&& dev_model : *device_models)
        {
            auto connected_devs_itr = std::find_if(begin(connected_devs), end(connected_devs),
                [&](const device& d) { return get_device_name(d) == get_device_name(dev_model->dev); });

            if (connected_devs_itr != end(connected_devs) || dev_model->dev.is<playback>())
                new_devices_count--;
        }


        ImGui::PushFont(window.get_font());

        int multiline_devices_names = 0;
        for (size_t i = 0; i < device_names.size(); i++)
        {
            if(device_names[i].first.find("\n") != std::string::npos)
            {
                bool show_device_in_list = true;
                for (auto&& dev_model : *device_models)
                {
                    if (get_device_name(dev_model->dev) == device_names[i])
                    {
                        show_device_in_list = false;
                        break;
                    }
                }
                if(show_device_in_list)
                {
                    multiline_devices_names++;
                }
            }
        }

        ImGui::SetNextWindowSize({ viewer_model.panel_width, 20.f * (new_devices_count + multiline_devices_names) + 8 + (is_ip_device_connected? 0 : 20)});
        if (ImGui::BeginPopup("select"))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, dark_grey);
            ImGui::Columns(2, "DevicesList", false);
            for (size_t i = 0; i < device_names.size(); i++)
            {
                bool skip = false;
                for (auto&& dev_model : *device_models)
                    if (get_device_name(dev_model->dev) == device_names[i]) skip = true;
                if (skip) continue;

                if (ImGui::Selectable(device_names[i].first.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)/* || switch_to_newly_loaded_device*/)
                {
                    try
                    {
                        auto dev = connected_devs[i];
                        device_models->emplace_back(new device_model(dev, error_message, viewer_model));
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
                if (auto ret = file_dialog_open(open_file, "ROS-bag\0*.bag\0", NULL, NULL))
                {
                    add_playback_device(ctx, *device_models, error_message, viewer_model, ret);
                }
            }
            ImGui::NextColumn();
            ImGui::Text("%s", "");
            ImGui::NextColumn();

            bool close_ip_popup = false;

            if (!is_ip_device_connected)
            {
                //ImGui::Separator();
                if (ImGui::Selectable("Add Network Device", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_DontClosePopups))
                {
#ifdef NETWORK_DEVICE
                    ip_address = config_file::instance().get_or_default(configurations::viewer::last_ip, std::string{});
                    ImGui::OpenPopup("Network Device");
#else
                    error_message = "To enable RealSense device over network, please build the SDK with CMake flag -DBUILD_NETWORK_DEVICE=ON.\nThis binary distribution was built with network features disabled.";
#endif
                }

                float width = 300;
                float height = 125;
                float posx = window.width() * 0.5f - width * 0.5f;
                float posy = window.height() * 0.5f - height * 0.5f;
                ImGui::SetNextWindowPos({ posx, posy });
                ImGui::SetNextWindowSize({ width, height });
                ImGui::PushStyleColor(ImGuiCol_PopupBg, sensor_bg);
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

                if (ImGui::BeginPopupModal("Network Device", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))    
                {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                    ImGui::SetCursorPosX(10);
                    ImGui::Text("Connect to a Linux system running rs-server");

                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    
                    bool connect = false;
                    static char ip_input[17];
                    std::copy(ip_address.begin(), ip_address.end(), ip_input);
                    ip_input[ip_address.size()] = '\0';
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    ImGui::SetCursorPosX(10);
                    ImGui::Text("Device IP: ");
                    ImGui::SameLine(); 
                    //ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
                    ImGui::PushItemWidth(width - ImGui::GetCursorPosX() - 10);
                    if (ImGui::GetWindowIsFocused() && 
                        !ImGui::IsAnyItemActive() && ip_address == "") 
                    {
                        ImGui::SetKeyboardFocusHere();
                    }
                    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue);


                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
                    if (ImGui::InputText("##ip", ip_input, 16, 
                        ImGuiInputTextFlags_CharsDecimal || 
                        ImGuiInputTextFlags_EnterReturnsTrue
                        ))
                    {
                        ip_address = ip_input;
                    }
                    ImGui::PopStyleColor();
                    
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

                    ImGui::PopItemWidth();
                    ImGui::SetCursorPosX(width / 2 - 105);

                    std::regex rgx("\\d{1,3}(\\.\\d{1,3}){3}");
                    if (!std::regex_match(ip_address,rgx))
                    {
                        ImGui::ButtonEx("OK",{100.f, 25.f}, ImGuiButtonFlags_Disabled);
                    }
                    else
                    {
                        if (ImGui::ButtonEx("OK",{100.f, 25.f}) || ImGui::IsKeyDown(GLFW_KEY_ENTER) || ImGui::IsKeyDown(GLFW_KEY_KP_ENTER))
                        {
                            try
                            {
                                is_ip_device_connected = add_remote_device(ctx, ip_address);;
                                refresh_devices(m, ctx, devices_connection_changes, connected_devs, device_names, *device_models, viewer_model, error_message);
                                auto dev = connected_devs[connected_devs.size()-1];
                                device_models->emplace_back(new device_model(dev, error_message, viewer_model));
                                config_file::instance().set(configurations::viewer::last_ip, ip_address);
                            }
                            catch (std::runtime_error e)
                            {
                                error_message = e.what();
                            }
                            ip_address = "";
                            close_ip_popup = true;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    ImGui::SameLine();
                    ImGui::SetCursorPosX(width / 2 + 5);
                    if(ImGui::Button("Cancel",{100.f, 25.f}) || ImGui::IsKeyDown(GLFW_KEY_ESCAPE))
                    {
                        ip_address = "";
                        close_ip_popup = true;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(1);
        
                ImGui::NextColumn();
                ImGui::Text("%s", "");
                ImGui::NextColumn();
            }

            if (close_ip_popup)
            {
                ImGui::CloseCurrentPopup();
                close_ip_popup = false;
            }
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


        viewer_model.show_top_bar(window, viewer_rect, *device_models);

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

        if (device_models->size() > 0)
        {
            std::vector<std::function<void()>> draw_later;
            auto windows_width = ImGui::GetContentRegionMax().x;

            for (auto&& dev_model : *device_models)
            {
                dev_model->draw_controls(viewer_model.panel_width, viewer_model.panel_y,
                    window, error_message, device_to_remove, viewer_model, windows_width, draw_later);
            }
            if (viewer_model.ppf.is_rendering())
            {
                if (!std::any_of(device_models->begin(), device_models->end(),
                    [](const std::unique_ptr<device_model>& dm)
                {
                    return dm->is_streaming();
                }))
                {
                    // Stopping post processing filter rendering thread
                    viewer_model.ppf.stop();
                }
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

            for (auto&& lambda : draw_later)
            {
                try
                {
                    lambda();
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

            if (device_to_remove)
            {
                if (auto p = device_to_remove->dev.as<playback>())
                {
                    ctx.unload_device(p.file_name());
                }
                viewer_model.syncer->remove_syncer(device_to_remove->dev_syncer);
                auto it = std::find_if(begin(*device_models), end(*device_models),
                    [&](const std::unique_ptr<device_model>& other)
                { return get_device_name(other->dev) == get_device_name(device_to_remove->dev); });

                if (it != device_models->end())
                {
                    it->reset();
                    device_models->erase(it);
                }

                device_to_remove = nullptr;
            }
        }
        else
        {
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            ImRect bb(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + ImGui::GetContentRegionAvail().y));
            ImGui::GetWindowDrawList()->AddRectFilled(bb.GetTL(), bb.GetBR(), ImColor(dark_window_background));

            viewer_model.show_no_device_overlay(window.get_large_font(), 50, static_cast<int>(viewer_model.panel_y + 50));
        }

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Fetch and process frames from queue
        viewer_model.handle_ready_frames(viewer_rect, window, static_cast<int>(device_models->size()), error_message);
    }

    // Stopping post processing filter rendering thread
    viewer_model.ppf.stop();

    // Stop all subdevices
    for (auto&& device_model : *device_models)
        for (auto&& sub : device_model->subdevices)
        {
            if (sub->streaming)
                sub->stop(viewer_model);
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
