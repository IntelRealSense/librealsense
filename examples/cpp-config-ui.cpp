#include <librealsense/rs2.hpp>
#include "example.hpp"
#include "model-views.h"

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

#pragma comment(lib, "opengl32.lib")

using namespace rs2;

struct user_data
{
    GLFWwindow* curr_window = nullptr;
    mouse_info* mouse = nullptr;
};

std::vector<std::string> get_device_info(const device& dev)
{
    std::vector<std::string> res;
    for (auto i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
    {
        auto info = static_cast<rs2_camera_info>(i);
        if (dev.supports(info))
        {
            auto value = dev.get_camera_info(info);
            res.push_back(value);

        }
    }
    return res;
}

std::string get_device_name(device& dev)
{
    // retrieve device name
    std::string name = (dev.supports(RS2_CAMERA_INFO_DEVICE_NAME))? dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME):"Unknown";

    // retrieve device serial number
    std::string serial = (dev.supports(RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER))? dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER):"Unknown";

    // retrieve device module name
    std::string module = (dev.supports(RS2_CAMERA_INFO_MODULE_NAME))? dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME):"Unknown";

    std::stringstream s;
    s<< std::setw(25)<<std::left<< name <<  " " <<std::setw(10)<<std::left<< module<<" Sn#" << serial;
    return s.str();        // push name and sn to list
}

std::vector<std::string> get_devices_names(const device_list& list)
{
    std::vector<std::string> device_names;

    for (uint32_t i = 0; i < list.size(); i++)
    {
        try
        {
            auto dev = list[i];
            device_names.push_back(get_device_name(dev));        // push name and sn to list
        }
        catch (...)
        {
            device_names.push_back(to_string() << "Unknown Device #" << i);
        }
    }
    return device_names;
}

int find_device_index(const device_list& list ,std::vector<std::string> device_info)
{
    std::vector<std::vector<std::string>> devices_info;

    for (auto l:list)
    {
        devices_info.push_back(get_device_info(l));
    }

    auto it = std::find(devices_info.begin(),devices_info.end(), device_info);
    return std::distance(devices_info.begin(), it);
}

int main(int, char**) try
{
    // activate logging to console
    log_to_console(RS2_LOG_SEVERITY_WARN);

    // Init GUI
    if (!glfwInit())
        exit(1);

    // Create GUI Windows
    auto window = glfwCreateWindow(1280, 720, "librealsense - config-ui", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(10, 0, 0);

    // Create RealSense Context
    context ctx;
    auto refresh_device_list = true;

    auto device_index = 0;

    std::vector<std::string> device_names;

    notifications_model not_model;
    std::string error_message{""};
    notification_model selected_notification;
    // Initialize list with each device name and serial number
    std::string label{""};

    mouse_info mouse;

    user_data data;
    data.curr_window = window;
    data.mouse = &mouse;


    glfwSetWindowUserPointer(window, &data);

    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double cx, double cy)
    {
        reinterpret_cast<user_data*>(glfwGetWindowUserPointer(w))->mouse->cursor = { (float)cx, (float)cy };
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods)
    {
        auto data = reinterpret_cast<user_data*>(glfwGetWindowUserPointer(w));
        data->mouse->mouse_down = (button == GLFW_MOUSE_BUTTON_1) && (action != GLFW_RELEASE);
    });

    auto last_time_point = std::chrono::high_resolution_clock::now();

    device dev;
    device_model model;
    device_list list;
    std::vector<std::string> active_device_info;
    auto dev_exist = false;
    auto hw_reset_enable = true;

    std::vector<device> devs;
    std::mutex m;

    auto timestamp =  std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();

    ctx.set_devices_changed_callback([&](event_information& info)
    {
        timestamp =  std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::lock_guard<std::mutex> lock(m);

        for(auto dev:devs)
        {
            if(info.was_removed(dev))
            {

                not_model.add_notification({get_device_name(dev) + "\ndisconnected",
                                            timestamp,
                                            RS2_LOG_SEVERITY_INFO,
                                            RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR});
            }
        }


        if(info.was_removed(dev))
        {
            dev_exist = false;
        }

        for(auto dev:info.get_new_devices())
        {
            not_model.add_notification({get_device_name(dev) + "\nconnected",
                                        timestamp,
                                        RS2_LOG_SEVERITY_INFO,
                                        RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR});
        }

        refresh_device_list = true;

    });

    // Closing the window
    while (!glfwWindowShouldClose(window))
    {
        {
            std::lock_guard<std::mutex> lock(m);

            if(refresh_device_list)
            {

                refresh_device_list = false;

                try
                {
                    list = ctx.query_devices();
                    
                    device_names = get_devices_names(list);

                    for(auto dev: devs)
                    {
                        dev = nullptr;
                    }
                    devs.clear();

                    if( !dev_exist)
                    {
                        device_index = 0;
                        dev = nullptr;
                        model.reset();
                       
                        if(list.size()>0)
                        {
                            dev = list[device_index];                  // Access first device
                            model = device_model(dev, error_message);  // Initialize device model
                            active_device_info = get_device_info(dev);
                            dev_exist = true;
                        }
                    }
                    else
                    {
                        device_index = find_device_index(list, active_device_info);
                    }

                    for (auto&& sub : list)
                    {
                        devs.push_back(sub);
                        sub.set_notifications_callback([&](const notification& n)
                        {
                            not_model.add_notification({n.get_description(), n.get_timestamp(), n.get_severity(), n.get_category()});
                        });
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

                hw_reset_enable = true;
            }
        }
        bool update_read_only_options = false;
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(now - last_time_point).count();
        if (duration >= 5000)
        {
            update_read_only_options = true;
            last_time_point = now;
        }

        glfwPollEvents();
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        ImGui_ImplGlfw_NewFrame();

        // Flags for pop-up window - no window resize, move or collaps
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        const float panel_size = 320;
        // Set window position and size
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ panel_size, static_cast<float>(h) });

        // *********************
        // Creating window menus
        // *********************
        ImGui::Begin("Control Panel", nullptr, flags);

        rs2_error* e = nullptr;
        label = to_string() << "VERSION: " << api_version_to_string(rs2_get_api_version(&e));
        ImGui::Text("%s", label.c_str());

        if (list.size() == 0)
        {
            ImGui::Text("No device detected.");
        }
        // Device Details Menu - Elaborate details on connected devices
        if (list.size() > 0 && ImGui::CollapsingHeader("Device Details", nullptr, true, true))
        {
            // Draw a combo-box with the list of connected devices


            auto new_index = device_index;
            if (model.draw_combo_box(device_names, new_index))
            {
                for (auto&& sub : model.subdevices)
                {
                    if (sub->streaming)
                        sub->stop();
                }

                try
                {
                    dev = list[new_index];
                    device_index = new_index;
                    model = device_model(dev, error_message);
                    active_device_info = get_device_info(dev);
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

            // Show all device details - name, module name, serial number, FW version and location
            model.draw_device_details(dev);
        }

        // Streaming Menu - Allow user to play different streams
        if (list.size()>0 && ImGui::CollapsingHeader("Streaming", nullptr, true, true))
        {
            if (model.subdevices.size() > 1)
            {
                try
                {
                    const float stream_all_button_width = 290;
                    const float stream_all_button_height = 0;

                    auto anything_stream = false;
                    for (auto&& sub : model.subdevices)
                    {
                        if (sub->streaming) anything_stream = true;
                    }
                    if (!anything_stream)
                    {
                        label = to_string() << "Start All";

                        if (ImGui::Button(label.c_str(), { stream_all_button_width, stream_all_button_height }))
                        {
                            for (auto&& sub : model.subdevices)
                            {
                                if (sub->is_selected_combination_supported())
                                {
                                    auto profiles = sub->get_selected_profiles();
                                    sub->play(profiles);

                                    for (auto&& profile : profiles)
                                    {
                                        model.streams[profile.stream].dev = sub;
                                    }
                                }
                            }

                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Start streaming from all subdevices");
                        }
                    }
                    else
                    {
                        label = to_string() << "Stop All";

                        if (ImGui::Button(label.c_str(), { stream_all_button_width, stream_all_button_height }))
                        {
                            for (auto&& sub : model.subdevices)
                            {
                                if (sub->streaming) sub->stop();
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Stop streaming from all subdevices");
                        }
                    }
                }
                catch(const error& e)
                {
                    error_message = error_to_string(e);
                }
                catch(const std::exception& e)
                {
                    error_message = e.what();
                }
            }

            // Draw menu foreach subdevice with its properties
            for (auto&& sub : model.subdevices)
            {

                label = to_string() << sub->dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME);
                if (ImGui::CollapsingHeader(label.c_str(), nullptr, true, true))
                {
                    sub->draw_stream_selection();

                    try
                    {
                        if (!sub->streaming)
                        {
                            label = to_string() << "Start " << sub->dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME);

                            if (sub->is_selected_combination_supported())
                            {
                                if (ImGui::Button(label.c_str()))
                                {
                                    auto profiles = sub->get_selected_profiles();
                                    sub->play(profiles);

                                    for (auto&& profile : profiles)
                                    {
                                        model.streams[profile.stream].dev = sub;
                                    }
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Start streaming data from selected sub-device");
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled("%s", label.c_str());
                            }
                        }
                        else
                        {
                            label = to_string() << "Stop " << sub->dev.get_camera_info(RS2_CAMERA_INFO_MODULE_NAME);
                            if (ImGui::Button(label.c_str()))
                            {
                                sub->stop();
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Stop streaming data from selected sub-device");
                            }
                        }
                    }
                    catch(const error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch(const std::exception& e)
                    {
                        error_message = e.what();
                    }

                    for (auto i = 0; i < RS2_OPTION_COUNT; i++)
                    {
                        auto opt = static_cast<rs2_option>(i);
                        auto&& metadata = sub->options_metadata[opt];
                        if (update_read_only_options)
                        {
                            metadata.update_supported(error_message);
                            if (metadata.supported && sub->streaming)
                            {
                                metadata.update_read_only(error_message);
                                if (metadata.read_only)
                                {
                                    metadata.update_all(error_message);
                                }
                            }
                        }
                        metadata.draw(error_message);
                    }
                }
                ImGui::Text("\n");
            }
        }

        if (list.size()>0 && ImGui::CollapsingHeader("Hardware Commands", nullptr, true))
        {
            label = to_string() << "Hardware Reset";

            const float hardware_reset_button_width = 290;
            const float hardware_reset_button_height = 0;

            if (ImGui::ButtonEx(label.c_str(), { hardware_reset_button_width, hardware_reset_button_height }, hw_reset_enable?0:ImGuiButtonFlags_Disabled))
            {
                try
                {
                    dev.hardware_reset();
                }
                catch(const error& e)
                {
                    error_message = error_to_string(e);
                }
                catch(const std::exception& e)
                {
                    error_message = e.what();
                }
            }
        }

        ImGui::Text("\n\n\n\n\n\n\n");

        for (auto&& sub : model.subdevices)
        {
            sub->update(error_message);
        }

        if (error_message != "")
            ImGui::OpenPopup("Oops, something went wrong!");
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

        not_model.draw(w, h, selected_notification);

        // Fetch frames from queue
        for (auto&& sub : model.subdevices)
        {
            for (auto& queue : sub->queues)
            {
                try
                {
                    frame f;
                    if (queue->poll_for_frame(&f))
                    {
                        model.upload_frame(std::move(f));
                    }
                }
                catch(const error& e)
                {
                    error_message = error_to_string(e);
                     sub->stop();
                }
                catch(const std::exception& e)
                {
                    error_message = e.what();
                    sub->stop();
                }
            }

        }

        // Rendering
        glViewport(0, 0,
                   static_cast<int>(ImGui::GetIO().DisplaySize.x),
                   static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(window, &w, &h);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, +1);

        auto layout = model.calc_layout(panel_size, 0.f, w - panel_size, (float)h);

        for (auto &&kvp : layout)
        {
            auto&& view_rect = kvp.second;
            auto stream = kvp.first;
            auto&& stream_size = model.streams[stream].size;
            auto stream_rect = view_rect.adjust_ratio(stream_size);

            model.streams[stream].show_frame(stream_rect, mouse, error_message);

            flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
            ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y });
            ImGui::SetNextWindowSize({ stream_rect.w, stream_rect.h });
            label = to_string() << "Stream of " << rs2_stream_to_string(stream);
            ImGui::Begin(label.c_str(), nullptr, flags);

            label = to_string() << rs2_stream_to_string(stream) << " "
                << stream_size.x << "x" << stream_size.y << ", "
                << rs2_format_to_string(model.streams[stream].format) << ", "
                << "Frame# " << model.streams[stream].frame_number << ", "
                << "FPS:";

            ImGui::Text("%s", label.c_str());
            ImGui::SameLine();

            label = to_string() << std::setprecision(2) << std::fixed << model.streams[stream].fps.get_fps();
            ImGui::Text("%s", label.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("FPS is calculated based on timestamps and not viewer time");
            }

            ImGui::SameLine((int)ImGui::GetWindowWidth() - 30);

            if (!layout.empty() && !model.fullscreen)
            {
                if (ImGui::Button("[+]", { 26, 20 }))
                {
                    model.fullscreen = true;
                    model.selected_stream = stream;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Maximize stream to full-screen");
                }
            }
            else if (model.fullscreen)
            {
                if (ImGui::Button("[-]", { 26, 20 }))
                {
                    model.fullscreen = false;
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Minimize stream to tile-view");
                }
            }

            if (!layout.empty())
            {
                if (model.streams[stream].roi_supported )
                {
                    ImGui::SameLine((int)ImGui::GetWindowWidth() - 160);
                    ImGui::Checkbox("[ROI]", &model.streams[stream].roi_checked);

                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Auto Exposure Region-of-Interest Selection");
                }
                else
                    model.streams[stream].roi_checked = false;
            }

            // Control metadata overlay widget
            ImGui::SameLine((int)ImGui::GetWindowWidth() - 90); // metadata GUI hint
            if (!layout.empty())
            {
                ImGui::Checkbox("[MD]", &model.streams[stream].metadata_displayed);

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Show per-frame metadata");
            }

            label = to_string() << "Timestamp: " << std::fixed << std::setprecision(3) << model.streams[stream].timestamp
                                << ", Domain:";
            ImGui::Text("%s", label.c_str());

            ImGui::SameLine();
            auto domain = model.streams[stream].timestamp_domain;
            label = to_string() << rs2_timestamp_domain_to_string(domain);

            if (domain == RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.0f, 0.0f, 1.0f });
                ImGui::Text("%s", label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hardware Timestamp unavailable! This is often an indication of inproperly applied Kernel patch.\nPlease refer to installation.md for mode information");
                }
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text("%s", label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Specifies the clock-domain for the timestamp (hardware-clock / system-time)");
                }
            }

            ImGui::End();
            ImGui::PopStyleColor();

            if (stream_rect.contains(mouse.cursor))
            {
                ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
                ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y + stream_rect.h - 25 });
                ImGui::SetNextWindowSize({ stream_rect.w, 25 });
                label = to_string() << "Footer for stream of " << rs2_stream_to_string(stream);
                ImGui::Begin(label.c_str(), nullptr, flags);

                std::stringstream ss;
                rect cursor_rect { mouse.cursor.x, mouse.cursor.y };
                auto ts = cursor_rect.normalize(stream_rect);
                auto&& stream_mv = model.streams[stream];
                auto pixels = ts.unnormalize(stream_mv._normalized_zoom.unnormalize(stream_mv.get_stream_bounds()));
                auto x = (int)pixels.x;
                auto y = (int)pixels.y;

                ss << std::fixed << std::setprecision(0) << x << ", " << y;

                float val{};
                if (stream_mv.texture->try_pick(x, y, &val))
                {
                    ss << ", *p: 0x" << std::hex << val;
                    if (stream == RS2_STREAM_DEPTH && val > 0)
                    {
                        auto meters = (val * stream_mv.dev->depth_units);
                        ss << std::dec << ", ~"
                           << std::setprecision(2) << meters << " meters";
                    }
                }

                label = ss.str();
                ImGui::Text("%s", label.c_str());

                ImGui::End();
                ImGui::PopStyleColor();
            }
        }

        // Metadata overlay windows shall be drawn after textures to preserve z-buffer functionality
        for (auto &&kvp : layout)
        {
            if (model.streams[kvp.first].metadata_displayed)
                model.streams[kvp.first].show_metadata(mouse);
        }

        ImGui::Render();


        glfwSwapBuffers(window);
    }

    // Stop all subdevices
    for (auto&& sub : model.subdevices)
    {
        if (sub->streaming)
            sub->stop();
    }

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

    return EXIT_SUCCESS;
}
catch (const error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
