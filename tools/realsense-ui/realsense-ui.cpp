#include <librealsense/rs2.hpp>
#include "example.hpp"
#include "model-views.h"
#include "../common/realsense-ui/realsense-ui-advanced-mode.h"

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

#pragma comment(lib, "opengl32.lib")

using namespace rs2;
using namespace rs400;

struct user_data
{
    GLFWwindow* curr_window = nullptr;
    mouse_info* mouse = nullptr;
};

class drag_drop_manager
{
    std::function<void(const std::string&)> device_added_handler = [](const std::string& s){ /*Do nothing*/};
    std::vector<std::string> files;
public:
    void add_device(const std::string& path)
    {
        if(std::find(files.begin(), files.end(), path) != files.end())
        {
            return; //already exists
        }
        files.push_back(path);
        device_added_handler(path);
    }

    void register_to_playback_device_added(std::function<void(const std::string&)> handle)
    {
        device_added_handler = handle;
    }

    static std::string get_file_name(const std::string& path)
    {
        std::string file_name;
        for (auto rit = path.rbegin(); rit != path.rend(); ++rit)
        {
            if(*rit == '\\' || *rit == '/')
                break;
            file_name += *rit;
        }
        std::reverse(file_name.begin(), file_name.end());
        return file_name;
    }
    void remove_device(const std::string& file)
    {
        auto it = std::find(files.begin(),files.end(), file);
        files.erase(it);
    }
};
drag_drop_manager drop_manager;

void handle_dropped_file(GLFWwindow* window, int count, const char** paths)
{
    if (count <= 0)
        return;

    for (int i = 0; i < count; i++)
    {
        drop_manager.add_device(paths[i]);
    }
}
std::vector<std::string> get_device_info(const device& dev, bool include_location = true)
{
    std::vector<std::string> res;
    for (auto i = 0; i < RS2_CAMERA_INFO_COUNT; i++)
    {
        auto info = static_cast<rs2_camera_info>(i);

        // When camera is being reset, either because of "hardware reset"
        // or because of switch into advanced mode,
        // we don't want to capture the info that is about to change
        if ((info == RS2_CAMERA_INFO_LOCATION ||
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

std::string get_device_name(device& dev)
{
    // retrieve device name
    std::string name = (dev.supports(RS2_CAMERA_INFO_NAME)) ? dev.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";

    // retrieve device serial number
    std::string serial = (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER)) ? dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "Unknown";

    std::stringstream s;

    if(dev.is<playback>())
    {
        auto playback_dev = dev.as<playback>();

        s << "Playback device: ";
        name += (to_string() << " (File: " << drag_drop_manager::get_file_name(playback_dev.file_name()) << ")");
    }
    s << std::setw(25) << std::left << name << " Sn# " << serial;
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

int find_device_index(const device_list& list, std::vector<std::string> device_info)
{
    std::vector<std::vector<std::string>> devices_info;

    for (auto l : list)
    {
        devices_info.push_back(get_device_info(l));
    }

    auto it = std::find(devices_info.begin(), devices_info.end(), device_info);
    return std::distance(devices_info.begin(), it);
}

void draw_general_tab(device_model& model, device_list& list,
    device& dev, std::string& label, bool hw_reset_enable,
    std::vector<std::string>& restarting_info,
    bool update_read_only_options, std::string& error_message)
{
    const float stream_all_button_width = 300;
    static bool is_recording = false;
    static char input_file_name[256] = "recorded_streams.bag";

    // Streaming Menu - Allow user to play different streams
    if ( (list.size()>0) && ImGui::CollapsingHeader("Streaming", nullptr, true, true))
    {
        if (model.subdevices.size() > 1)
        {
            try
            {
                auto anything_stream = std::any_of(model.subdevices.begin(), model.subdevices.end(), [](std::shared_ptr<subdevice_model> sub) {
                    return sub->streaming;
                });
                if (!anything_stream)
                {
                    label = to_string() << "Start All";
                    if (ImGui::Button(label.c_str(), { stream_all_button_width, 0 }))
                    {
                        if (is_recording)
                        {
							model.start_recording(dev, input_file_name, error_message);
                        }
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
                    ImGui::Checkbox("Enable Recording", &is_recording);
                    if (is_recording)
                    {
                        ImGui::Text("Save to:"); ImGui::InputText("file_path", input_file_name, 256, ImGuiInputTextFlags_CharsNoBlank);
                    }
                }
                else
                {
                    label = to_string() << "Stop All";

                    if (ImGui::Button(label.c_str(), { stream_all_button_width / 2 - 5, 0 }))
                    {
                        for (auto&& sub : model.subdevices)
                        {
                            if (sub->streaming) sub->stop();
                        }
                        if (is_recording)
                        {
                            model.stop_recording();
                            for (auto&& sub : model.subdevices)
                            {
                                if (sub->is_selected_combination_supported())
                                {
                                    auto profiles = sub->get_selected_profiles();
                                    for (auto&& profile : profiles)
                                    {
                                        model.streams[profile.stream].dev = sub;
                                    }
                                }
                            }
                        }
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Stop streaming from all subdevices");
                    }

                    ImGui::SameLine();

                    bool any_paused = false;
                    for (auto&& sub : model.subdevices)
                    {
                        if (sub->streaming && sub->is_paused())
                            any_paused = true;
                    }

                    if (any_paused)
                    {
                        label = to_string() << "Resume All";

                        if (ImGui::Button(label.c_str(), { stream_all_button_width / 2, 0 }))
                        {
                            for (auto&& sub : model.subdevices)
                            {
                                if (sub->streaming) sub->resume();
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Resume streaming live data from all sub-devices");
                        }
                    }
                    else
                    {
                        label = to_string() << "Pause All";

                        if (ImGui::Button(label.c_str(), { stream_all_button_width / 2, 0 }))
                        {
                            for (auto&& sub : model.subdevices)
                            {
                                if (sub->streaming) sub->pause();
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Freeze the UI on the current frame. The camera will continue to work in the background");
                        }
                    }
                    if (ImGui::CollapsingHeader("Recording Options", &is_recording))
                    {
                        static bool is_paused = false;
                        if (!is_paused && ImGui::Button("Pause"))
                        {
                            model.pause_record();
                            is_paused = !is_paused;
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Pause recording to file, Streaming will continue");
                        }
                        if (is_paused && ImGui::Button("Resume"))
                        {
                            model.resume_record();
                            is_paused = !is_paused;
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Continue recording");
                        }
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
        }

        // Draw menu foreach subdevice with its properties
        for (auto&& sub : model.subdevices)
        {

            label = to_string() << sub->s.get_info(RS2_CAMERA_INFO_NAME);
            if (ImGui::CollapsingHeader(label.c_str(), nullptr, true, true))
            {
                sub->draw_stream_selection();

                try
                {
                    if (!sub->streaming)
                    {
                        label = to_string() << "Start " << sub->s.get_info(RS2_CAMERA_INFO_NAME);

                        if (sub->is_selected_combination_supported())
                        {
                            if (ImGui::Button(label.c_str(), { stream_all_button_width, 0 }))
                            {
                                if (is_recording)
                                {
                                    model.start_recording(dev, input_file_name, error_message);
                                }
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
                        label = to_string() << "Stop##" << sub->s.get_info(RS2_CAMERA_INFO_NAME);
                        if (ImGui::Button(label.c_str(), { stream_all_button_width / 2 - 5, 0 }))
                        {
                            sub->stop();
                            if (is_recording)
                            {
                                auto streaming_sensors_count = std::count_if(model.subdevices.begin(),
                                                                     model.subdevices.end(),
                                                                     [](std::shared_ptr<subdevice_model> sub)
                                                                     {
                                                                         return sub->streaming;
                                                                     });
                                if (streaming_sensors_count == 0)
                                {
                                    //TODO: move this inside model
                                    model.stop_recording();
                                    for (auto&& sub : model.subdevices)
                                    {
                                        if (sub->is_selected_combination_supported())
                                        {
                                            auto profiles = sub->get_selected_profiles();
                                            for (auto&& profile : profiles)
                                            {
                                                model.streams[profile.stream].dev = sub;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Stop streaming data from selected sub-device");
                        }

                        ImGui::SameLine();

                        if (sub->is_paused())
                        {
                            label = to_string() << "Resume >>##" << sub->s.get_info(RS2_CAMERA_INFO_NAME);
                            if (ImGui::Button(label.c_str(), { stream_all_button_width / 2 - 5, 0 }))
                            {
                                sub->resume();
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Resume live streaming from the sub-device");
                            }
                        }
                        else
                        {
                            label = to_string() << "Pause ||##" << sub->s.get_info(RS2_CAMERA_INFO_NAME);
                            if (ImGui::Button(label.c_str(), { stream_all_button_width / 2 - 5, 0 }))
                            {
                                sub->pause();
                            }
                            if (ImGui::IsItemHovered())
                            {
                                ImGui::SetTooltip("Freeze current frame. The sub-device will continue to work in the background");
                            }
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

                static const std::vector<rs2_option> options_order{RS2_OPTION_ADVANCED_MODE_PRESET,
                                                                   RS2_OPTION_ENABLE_AUTO_EXPOSURE,
                                                                   RS2_OPTION_EXPOSURE,
                                                                   RS2_OPTION_EMITTER_ENABLED,
                                                                   RS2_OPTION_LASER_POWER};
                sub->draw_options(options_order, update_read_only_options, error_message);

                auto&& de_opt = sub->options_metadata[RS2_OPTION_DEPTH_UNITS];
                if (de_opt.supported)
                {
                    if (ImGui::Checkbox("Histogram Equalization", &model.streams[RS2_STREAM_DEPTH].texture->equalize))
                    {
                        auto depth_units = de_opt.value;
                        model.streams[RS2_STREAM_DEPTH].texture->min_depth = 0;
                        model.streams[RS2_STREAM_DEPTH].texture->max_depth = 6 / depth_units;
                    }
                    if (!model.streams[RS2_STREAM_DEPTH].texture->equalize)
                    {
                        auto depth_units = de_opt.value;
                        auto val = model.streams[RS2_STREAM_DEPTH].texture->min_depth * depth_units;
                        if (ImGui::SliderFloat("Near (m)", &val, 0, 16))
                        {
                            model.streams[RS2_STREAM_DEPTH].texture->min_depth = val / depth_units;
                        }
                        val = model.streams[RS2_STREAM_DEPTH].texture->max_depth * depth_units;
                        if (ImGui::SliderFloat("Far  (m)", &val, 0, 16))
                        {
                            model.streams[RS2_STREAM_DEPTH].texture->max_depth = val / depth_units;
                        }
                        if (model.streams[RS2_STREAM_DEPTH].texture->min_depth > model.streams[RS2_STREAM_DEPTH].texture->max_depth)
                        {
                            std::swap(model.streams[RS2_STREAM_DEPTH].texture->max_depth, model.streams[RS2_STREAM_DEPTH].texture->min_depth);
                        }
                    }
                }
            }
            ImGui::Text("\n");
        }
    }

    if (list.size() > 0 && ImGui::CollapsingHeader("Hardware Commands", nullptr, true, true))
    {
        label = to_string() << "Hardware Reset";

        const float hardware_reset_button_width = 300;
        const float hardware_reset_button_height = 0;

        if (ImGui::ButtonEx(label.c_str(), { hardware_reset_button_width, hardware_reset_button_height }, hw_reset_enable ? 0 : ImGuiButtonFlags_Disabled))
        {
            try
            {
                restarting_info = get_device_info(dev, false);
                dev.hardware_reset();
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
            ImGui::SetTooltip("Ask camera firmware to restart the device");
        }
    }

    ImGui::Text("\n\n\n\n\n\n\n");

    for (auto&& sub : model.subdevices)
    {
        sub->update(error_message);
    }
}

void draw_advanced_mode_tab(device& dev, advanced_mode_control& amc,
    std::vector<std::string>& restarting_info,
    bool& get_curr_advanced_controls)
{
    auto is_advanced_mode = dev.is<advanced_mode>();

    if (ImGui::CollapsingHeader("Advanced Mode", nullptr, true, true))
    {
        try
        {
            if (!is_advanced_mode)
            {
                // TODO: Why are we showing the tab then??
                ImGui::TextColored(ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f }, "Selected device does not offer\nany advanced settings");
            }

            auto advanced = dev.as<advanced_mode>();
            if (advanced.is_enabled())
            {
                if (ImGui::Button("Disable Advanced Mode", ImVec2{ 290, 0 }))
                {
                    //if (yes_no_dialog()) // TODO
                    //{
                    advanced.toggle_advanced_mode(false);
                    restarting_info = get_device_info(dev, false);
                    //}
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Disabling advanced mode will reset depth generation to factory settings\nThis will not affect calibration");
                }
                draw_advanced_mode_controls(advanced, amc, get_curr_advanced_controls);
            }
            else
            {
                if (ImGui::Button("Enable Advanced Mode", ImVec2{ 300, 0 }))
                {
                    //if (yes_no_dialog()) // TODO
                    //{
                    advanced.toggle_advanced_mode(true);
                    restarting_info = get_device_info(dev, false);
                    //}
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Advanced mode is a persistent camera state unlocking calibration formats and depth generation controls\nYou can always reset the camera to factory defaults by disabling advanced mode");
                }
                ImGui::TextColored(ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f }, "Device is not in advanced mode!\nTo access advanced functionality\nclick \"Enable Advanced Mode\"");
            }
        }
        catch (...)
        {
            // TODO
        }
    }
}

int main(int, char**) try
{
    // activate logging to console
    log_to_console(RS2_LOG_SEVERITY_WARN);

    // Init GUI
    if (!glfwInit())
        exit(1);

    // Create GUI Windows
    auto window = glfwCreateWindow(1280, 720, "RealSense Viewer", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(10, 0, 0);

    // Create RealSense Context
    context ctx;
    auto refresh_device_list = true;
    std::vector<std::string> restarting_device_info;

    auto device_index = 0;

    std::vector<std::string> device_names;

    // The list of errors the user asked not to show again:
    std::set<std::string> errors_not_to_show;
    bool dont_show_this_error = false;
    auto simplify_error_message = [](const std::string& s) {
        std::regex e("\\b(0x)([^ ,]*)");
        return std::regex_replace(s, e, "address");
    };

    notifications_model not_model;
    std::string error_message{ "" };
    notification_model selected_notification;
    // Initialize list with each device name and serial number
    std::string label{ "" };

    drop_manager.register_to_playback_device_added([&refresh_device_list, &not_model, &ctx](const std::string& path)
   {
       try
       {
           auto p = ctx.load_device(path);
       }
       catch(rs2::error& e)
       {
           not_model.add_notification({to_string() << "Failed to create playback from file: " << path << ". Reason: " << e.what(),
                                       std::chrono::duration_cast<std::chrono::duration<double,std::micro>>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(),
                                       RS2_LOG_SEVERITY_ERROR,
                                       RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR});
       }
   });

    mouse_info mouse;

    user_data data;
    data.curr_window = window;
    data.mouse = &mouse;
    
    glfwSetDropCallback(window, handle_dropped_file);
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

    auto timestamp = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();

    ctx.set_devices_changed_callback([&](event_information& info)
    {
        timestamp = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::lock_guard<std::mutex> lock(m);

        for (auto dev : devs)
        {
            if (info.was_removed(dev))
            {
                not_model.add_notification({ get_device_name(dev) + " Disconnected\n",
                    timestamp,
                    RS2_LOG_SEVERITY_INFO,
                    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });

                if(dev.is<playback>())
                {
                    drop_manager.remove_device(dev.as<playback>().file_name());
                }
            }
        }


        if (info.was_removed(dev))
        {
            dev_exist = false;
        }

        try
        {
            for (auto dev : info.get_new_devices())
            {
                not_model.add_notification({ get_device_name(dev) + " Connected\n",
                    timestamp,
                    RS2_LOG_SEVERITY_INFO,
                    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }
        }
        catch (...)
        {

        }
        refresh_device_list = true;
    });

    advanced_mode_control amc{};
    bool get_curr_advanced_controls = true;

    int last_tab_index = 0;
    int tab_index = 0;

    int last_preset_index = 0;
    int preset_index = -1;

    // Closing the window
    while (!glfwWindowShouldClose(window))
    {
        {
            std::lock_guard<std::mutex> lock(m);

            if (refresh_device_list)
            {
                refresh_device_list = false;

                try
                {
                    list = ctx.query_devices();

                    device_names = get_devices_names(list);
                    
                    for (auto dev : devs)
                    {
                        dev = nullptr;
                    }
                    devs.clear();

                    if (!dev_exist)
                    {
                        device_index = 0;
                        dev = nullptr;
                        model.reset();

                        if(list.size() > 0)
                        {
                            dev = list[device_index];                  // Access first device
                            model = device_model(dev, error_message);  // Initialize device model
                            active_device_info = get_device_info(dev);
                            dev_exist = true;
                            get_curr_advanced_controls = true;
                        }
                    }
                    else
                    {
                        device_index = find_device_index(list, active_device_info);
                    }

                    for (size_t i = 0; i < list.size(); i++)
                    {
                        auto&& d = list[i];
                        auto info = get_device_info(d, false);
                        if (info == restarting_device_info)
                        {
                            device_index = i;
                            model.reset();

                            dev = d;
                            model = device_model(dev, error_message);
                            active_device_info = get_device_info(dev);
                            dev_exist = true;
                            restarting_device_info.clear();
                            get_curr_advanced_controls = true;
                        }
                    }

                    for (auto&& sub : list)
                    {
                        devs.push_back(sub);
                        for (auto&& s : sub.query_sensors())
                        {
                            s.set_notifications_callback([&](const notification& n)
                            {
                                not_model.add_notification({ n.get_description(), n.get_timestamp(), n.get_severity(), n.get_category() });
                            });
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

                hw_reset_enable = true;
            }
        }
        bool update_read_only_options = false;
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(now - last_time_point).count();
        if (duration >= 6000)
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
        bool any_device_exists = (list.size() > 0);
        if (any_device_exists)
        {
            // Draw 3 tabs
            const char* tabs[] = { "General", "Advanced" };
            if (ImGui::TabLabels(tabs, 2, tab_index))
                last_tab_index = tab_index;
        }
        else
        {
            ImGui::Text("No device detected.");
        }
        // Device Details Menu - Elaborate details on connected devices
        if (any_device_exists > 0 && ImGui::CollapsingHeader("Device Details", nullptr, true, true))
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
            if(dev.is<playback>())
            {
                auto p = dev.as<playback>();
                if (ImGui::SmallButton("Remove Device"))
                {
                    ctx.unload_device(p.file_name());
                }
                else
                {
                    static int seek_progress = 0;
                    static float progress = 0;
                    //TOOD: p.get_position();
                    seek_progress = static_cast<int>(std::max(0.0f, std::min(progress, 1.0f)) * 100);

                    int prev_seek_progress = seek_progress;

                    ImGui::SeekSlider("Seek Bar", &seek_progress);
                    if (prev_seek_progress != seek_progress)
                    {
                        //Seek was dragged
                        //TODO: auto duration = p.get_duration();
                        //TODO: auto single_percent = duration.count / 100;
                        //TODO: p.seek(std::chrono::nanoseconds(seek_progress * single_percent));
                        progress = static_cast<float>(seek_progress / 100.0f);
                    }
                }

            }
        }


        if (list.size() > 0)
        {
            if (last_tab_index == 0)
            {
                draw_general_tab(model, list, dev, label, hw_reset_enable, restarting_device_info, update_read_only_options, error_message);
            }
            else if (last_tab_index == 1)
            {
                draw_advanced_mode_tab(dev, amc, restarting_device_info, get_curr_advanced_controls);
            }
        }


        if (error_message != "")
        {
            if (errors_not_to_show.count(simplify_error_message(error_message)))
            {
                not_model.add_notification({ error_message,
                    std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count(),
                    RS2_LOG_SEVERITY_ERROR,
                    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
                error_message = "";
            }
            else
            {
                ImGui::OpenPopup("Oops, something went wrong!");
            }
        }
        if (ImGui::BeginPopupModal("Oops, something went wrong!", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("RealSense error calling:");
            ImGui::InputTextMultiline("error", const_cast<char*>(error_message.c_str()),
                error_message.size() + 1, { 500,100 }, ImGuiInputTextFlags_AutoSelectAll);
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                if (dont_show_this_error)
                {
                    errors_not_to_show.insert(simplify_error_message(error_message));
                }
                error_message = "";
                ImGui::CloseCurrentPopup();
                dont_show_this_error = false;
            }

            ImGui::SameLine();
            ImGui::Checkbox("Don't show this error again", &dont_show_this_error);

            ImGui::EndPopup();
        }

        ImGui::End();

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
                catch (const error& e)
                {
                    error_message = error_to_string(e);
                    sub->stop();
                }
                catch (const std::exception& e)
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
            ImGui::SetNextWindowSize({ stream_rect.w, 50 });
            label = to_string() << "Stream of " << rs2_stream_to_string(stream);
            ImGui::Begin(label.c_str(), nullptr, flags);

            if (model.streams[stream].show_stream_details)
            {
                label = to_string() << rs2_stream_to_string(stream) << " "
                    << stream_size.x << "x" << stream_size.y << ", "
                    << rs2_format_to_string(model.streams[stream].format) << ", "
                    << "Frame# " << model.streams[stream].frame_number << ", "
                    << "FPS:";
            }
            else
            {
                label = to_string() << rs2_stream_to_string(stream) << " (...)";
            }

            ImGui::Text("%s", label.c_str());
            model.streams[stream].show_stream_details = ImGui::IsItemHovered();

            if (model.streams[stream].show_stream_details)
            {
                ImGui::SameLine();

                label = to_string() << std::setprecision(2) << std::fixed << model.streams[stream].fps.get_fps();
                ImGui::Text("%s", label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("FPS is calculated based on timestamps and not viewer time");
                }
            }

            ImGui::SameLine((int)ImGui::GetWindowWidth() - 35);

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


            // Control metadata overlay widget
            ImGui::SameLine((int)ImGui::GetWindowWidth() - 140); // metadata GUI hint
            if (!layout.empty())
            {
                if (model.streams[stream].metadata_displayed)
                {
                    if (ImGui::Button("Hide Metadata", { 100, 20 }))
                        model.streams[stream].metadata_displayed = false;
                }
                else
                {
                    if (ImGui::Button("Show Metadata", { 100, 20 }))
                        model.streams[stream].metadata_displayed = true;
                }

                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Show per-frame metadata");
            }

            if (model.streams[stream].show_stream_details)
            {
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
            }

            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
            ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y + stream_rect.h - 30 });
            ImGui::SetNextWindowSize({ stream_rect.w, 30 });
            label = to_string() << "Footer for stream of " << rs2_stream_to_string(stream);
            ImGui::Begin(label.c_str(), nullptr, flags);

            auto&& stream_mv = model.streams[stream];

            if (stream_rect.contains(mouse.cursor))
            {
                std::stringstream ss;
                rect cursor_rect{ mouse.cursor.x, mouse.cursor.y };
                auto ts = cursor_rect.normalize(stream_rect);
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
            }

            // Since applying color map involve some actual processing on the incoming frame
            // we can't change the color map when the stream is frozen
            if (stream == RS2_STREAM_DEPTH && stream_mv.dev && !stream_mv.dev->is_paused())
            {
                ImGui::SameLine((int)ImGui::GetWindowWidth() - 150);
                ImGui::PushItemWidth(-1);
                if (ImGui::Combo("Color Map:", &model.streams[stream].color_map_idx, color_maps_names.data(), color_maps_names.size()))
                {
                    model.streams[stream].texture->cm = color_maps[model.streams[stream].color_map_idx];
                }
                ImGui::PopItemWidth();
            }

            ImGui::End();
            ImGui::PopStyleColor();

        }

        // Metadata overlay windows shall be drawn after textures to preserve z-buffer functionality
        for (auto &&kvp : layout)
        {
            if (model.streams[kvp.first].metadata_displayed)
                model.streams[kvp.first].show_metadata(mouse);
        }

        not_model.draw(w, h, selected_notification);

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
