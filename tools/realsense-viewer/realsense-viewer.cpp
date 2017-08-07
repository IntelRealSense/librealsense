#include <librealsense/rs2.hpp>
#include "example.hpp"
#include "parser.hpp"
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

#include <imgui_internal.h>

#define NOC_FILE_DIALOG_IMPLEMENTATION
#include <noc_file_dialog.h>

#define ARCBALL_CAMERA_IMPLEMENTATION
#include "arcball_camera.h"

#pragma comment(lib, "opengl32.lib")

using namespace rs2;
using namespace rs400;



void show_3dviewer_header(ImFont* font, rs2::rect stream_rect, viewer_model& viewer)
{
    const auto top_bar_height = 32.f;

    auto flags = ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar;

    ImGui::PushFont(font);
    ImGui::PushStyleColor(ImGuiCol_Text, light_grey);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

    ImGui::PushStyleColor(ImGuiCol_Button, header_window_bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, header_window_bg);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, header_window_bg);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, header_window_bg);
    ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y });
    ImGui::SetNextWindowSize({ stream_rect.w, top_bar_height });
    std::string label = to_string() << "header of 3dviewer";
    ImGui::Begin(label.c_str(), nullptr, flags);



    ImGui::End();
    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar();
    ImGui::PopFont();
}

struct user_data
{
    GLFWwindow* curr_window = nullptr;
    mouse_info* mouse = nullptr;
};

class drag_drop_manager
{
    std::function<void(const std::string&)> device_added_handler = [](const std::string& s) { /*Do nothing*/};
    std::vector<std::string> files;
public:
    void add_device(const std::string& path)
    {
        if (std::find(files.begin(), files.end(), path) != files.end())
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
            if (*rit == '\\' || *rit == '/')
                break;
            file_name += *rit;
        }
        std::reverse(file_name.begin(), file_name.end());
        return file_name;
    }
    void remove_device(const std::string& file)
    {
        auto it = std::find(files.begin(), files.end(), file);
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

std::vector<std::pair<std::string, std::string>> get_devices_names(const device_list& list)
{
    std::vector<std::pair<std::string, std::string>> device_names;

    for (uint32_t i = 0; i < list.size(); i++)
    {
        try
        {
            auto dev = list[i];
            device_names.push_back(get_device_name(dev));        // push name and sn to list
        }
        catch (...)
        {
            device_names.push_back(std::pair<std::string, std::string>(to_string() << "Unknown Device #" << i, ""));
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

void draw_advanced_mode_tab(device& dev, advanced_mode_control& amc,
    std::vector<std::string>& restarting_info,
    bool& get_curr_advanced_controls)
{
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 0.9f, 0.9f, 0.9f, 1 });

    auto is_advanced_mode = dev.is<advanced_mode>();
    if (ImGui::TreeNode("Advanced Mode"))
    {
        try
        {
            if (!is_advanced_mode)
            {
                // TODO: Why are we showing the tab then??
                ImGui::TextColored(ImVec4{ 1.0f, 0.0f, 0.0f, 1.0f }, "Selected device does not offer\nany advanced settings");
            }
            else
            {
                auto advanced = dev.as<advanced_mode>();
                if (advanced.is_enabled())
                {
                    if (ImGui::Button("Disable Advanced Mode", ImVec2{ 180, 0 }))
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
                    if (ImGui::Button("Enable Advanced Mode", ImVec2{ 180, 0 }))
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
        }
        catch (...)
        {
            // TODO
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleColor();
}

void reset_camera(float3& pos, float3& target, float3& up)
{
    pos = { 0.0f, 0.0f, -1.5f };
    target = { 0.0f, 0.0f, 0.0f };

    // initialize "up" to be tangent to the sphere!
    // up = cross(cross(look, world_up), look)
    {
        float3 look = { target.x - pos.x, target.y - pos.y, target.z - pos.z };
        look = look.normalize();

        float world_up[3] = { 0.0f, 1.0f, 0.0f };

        float across[3] = {
            look.y * world_up[2] - look.z * world_up[1],
            look.z * world_up[0] - look.x * world_up[2],
            look.x * world_up[1] - look.y * world_up[0],
        };

        up.x = across[1] * look.z - across[2] * look.y;
        up.y = across[2] * look.x - across[0] * look.z;
        up.z = across[0] * look.y - across[1] * look.x;

        float up_len = up.length();
        up.x /= -up_len;
        up.y /= -up_len;
        up.z /= -up_len;
    }
}

int main(int, char**) try
{
    // activate logging to console
    log_to_console(RS2_LOG_SEVERITY_WARN);

    // Init GUI
    if (!glfwInit())
        exit(1);

    rs2_error* e = nullptr;
    std::string title = to_string() << "RealSense Viewer v" << api_version_to_string(rs2_get_api_version(&e));

    // Create GUI Windows
    auto window = glfwCreateWindow(1280, 720, title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImFont* font_18;
    ImFont* font_14;

    imgui_easy_theming(font_14, font_18);

    ImVec4 clear_color = ImColor(10, 0, 0);

    // Create RealSense Context
    context ctx;
    auto refresh_device_list = true;
    std::vector<std::string> restarting_device_info;

    bool is_3d_view = false;
    bool anything_started = false;

    std::vector<std::pair<std::string, std::string>> device_names;

    std::string error_message{ "" };

    std::string label{ "" };
    bool new_device_loaded = false;


    auto last_time_point = std::chrono::high_resolution_clock::now();

    std::vector<device_model> device_models;
    device_model* device_to_remove = nullptr;

    viewer_model viewer_model;
    device_list list;
    std::vector<std::string> active_device_info;
    auto dev_exist = false;
    auto hw_reset_enable = true;
    auto pc = ctx.create_pointcloud();
    int rendered_tex_id = 0;
    std::atomic<bool> keep_calculating_pointcloud = true;
    frame_queue depth_frames_to_render(1);
    frame_queue resulting_3d_models(1);
    bool paused = false;

    std::thread pointcloud_thread([&]() {
        while (keep_calculating_pointcloud)
        {
            frame f;
            if (depth_frames_to_render.poll_for_frame(&f))
            {
                resulting_3d_models.enqueue(pc.calculate(f));
            }
        }
    });

    std::vector<device> devs;
    std::mutex m;

    auto timestamp = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();

    mouse_info mouse;

    user_data data;
    data.curr_window = window;
    data.mouse = &mouse;

    glfwSetDropCallback(window, handle_dropped_file);
    glfwSetWindowUserPointer(window, &data);

    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double cx, double cy)
    {
        auto data = reinterpret_cast<user_data*>(glfwGetWindowUserPointer(w));
        data->mouse->cursor = { (float)cx, (float)cy };
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods)
    {
        auto data = reinterpret_cast<user_data*>(glfwGetWindowUserPointer(w));
        data->mouse->mouse_down = (button == GLFW_MOUSE_BUTTON_1) && (action != GLFW_RELEASE);
    });
    glfwSetScrollCallback(window, [](GLFWwindow * w, double xoffset, double yoffset)
    {
        auto data = reinterpret_cast<user_data*>(glfwGetWindowUserPointer(w));
        data->mouse->mouse_wheel = yoffset;
    });

    //ctx.set_devices_changed_callback([&](event_information& info)
    //{
    //    timestamp = std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count();

    //    std::lock_guard<std::mutex> lock(m);

    //    for (auto dev : devs)
    //    {
    //        if (info.was_removed(dev))
    //        {
    //            viewer_model.not_model.add_notification({ get_device_name(dev).first + " Disconnected\n",
    //                timestamp,
    //                RS2_LOG_SEVERITY_INFO,
    //                RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });

    //            if (dev.is<playback>())
    //            {
    //                drop_manager.remove_device(dev.as<playback>().file_name());
    //            }
    //        }
    //    }


    //    if (info.was_removed(dev))
    //    {
    //        dev_exist = false;
    //    }

    //    try
    //    {
    //        for (auto dev : info.get_new_devices())
    //        {
    //            viewer_model.not_model.add_notification({ get_device_name(dev).first + " Connected\n",
    //                timestamp,
    //                RS2_LOG_SEVERITY_INFO,
    //                RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
    //        }
    //    }
    //    catch (...)
    //    {

    //    }
    //    refresh_device_list = true;
    //});

    // 3D-Viewer state
    float3 pos = { 0.0f, 0.0f, -0.5f };
    float3 target = { 0.0f, 0.0f, 0.0f };
    float3 up;
    float view[16];

    reset_camera(pos, target, up);
    bool texture_wrapping_on = true;
    GLint texture_border_mode = GL_CLAMP_TO_EDGE; // GL_CLAMP_TO_BORDER
    float tex_border_color[] = { 0.8f, 0.8f, 0.8f, 0.8f };
    float2 oldcursor{ 0.f, 0.f };

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

                    //for (auto dev : devs)
                    //{
                    //    dev = nullptr;
                    //}
                    //devs.clear();

                    //if (!dev_exist)
                    //{
                    //    device_index = 0;
                    //    dev = nullptr;
                    //    device_model.reset();

                    //    if (list.size() > 0)
                    //    {
                    //        dev = list[device_index];                  // Access first device
                    //        device_model = device_model(dev, error_message);  // Initialize device model
                    //        viewer_model.not_model.add_log(to_string() << dev.get_info(RS2_CAMERA_INFO_NAME) << " was selected as default device");
                    //        active_device_info = get_device_info(dev);
                    //        dev_exist = true;
                    //        get_curr_advanced_controls = true;
                    //    }
                    //}
                    //else
                    //{
                    //    device_index = find_device_index(list, active_device_info);
                    //}

                    //for (size_t i = 0; i < list.size(); i++)
                    //{
                    //    auto&& d = list[i];
                    //    auto info = get_device_info(d, false);
                    //    if (info == restarting_device_info)
                    //    {
                    //        device_index = i;
                    //        device_model.reset();

                    //        dev = d;
                    //        device_model = device_model(dev, error_message);
                    //        active_device_info = get_device_info(dev);
                    //        dev_exist = true;
                    //        restarting_device_info.clear();
                    //        get_curr_advanced_controls = true;
                    //    }
                    //}

                    for (auto&& sub : list)
                    {
                        devs.push_back(sub);
                        for (auto&& s : sub.query_sensors())
                        {
                            s.set_notifications_callback([&](const notification& n)
                            {
                                viewer_model.not_model.add_notification({ n.get_description(), n.get_timestamp(), n.get_severity(), n.get_category() });
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
        auto view_clock = std::chrono::high_resolution_clock::now();
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
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings;

        const float panel_width = 320.f;
        const float panel_y = 44.f;
        const float default_log_h = 80.f;

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ panel_width, panel_y });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Add Device Panel", nullptr, flags);

        //bool switch_to_newly_loaded_device = false;
        //if (new_device_loaded && refresh_device_list == false)
        //{
        //    device_index = list.size() - 1;
        //    new_device_loaded = false;
        //    switch_to_newly_loaded_device = true;
        //}

        ImGui::PushFont(font_18);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, from_rgba(230, 230, 230, 255));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, from_rgba(0, 0xae, 0xff, 255));
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, from_rgba(255, 255, 255, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
        ImGui::SetNextWindowPos({ 0, panel_y });

        if (ImGui::Button(u8"Add Source\t\t\t\t\t\t\t\t\t\t\t\t\t\t\uf055", { panel_width - 1, panel_y }))
            ImGui::OpenPopup("select");

        auto new_devices_count = device_names.size() + 1;
        for (auto&& dev_model : device_models)
            if (list.contains(dev_model.dev))
                new_devices_count--;

        ImGui::PushFont(font_14);
        ImGui::SetNextWindowSize({ panel_width, 20.f * new_devices_count + 8 });
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
                        auto model = device_model(dev, error_message);
                        device_models.push_back(model);
                        active_device_info = get_device_info(dev);
                        if (dev.is<playback>()) //TODO: remove this and make sub->streaming a function that queries the sensor
                        {
                            dev.as<playback>().set_status_changed_callback([&model](rs2_playback_status status)
                            {
                                if (status == RS2_PLAYBACK_STATUS_STOPPED)
                                {
                                    for (auto sub : model.subdevices)
                                    {
                                        if (sub->streaming)
                                        {
                                            sub->stop();
                                        }
                                    }
                                }
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

            if (ImGui::Selectable("Recording from File...", false, ImGuiSelectableFlags_SpanAllColumns))
            {
                const char *ret;
                ret = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN,
                    "ROS-bag\0*.bag\0", NULL, NULL);

            }
            ImGui::NextColumn();
            ImGui::Text("");
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


        ImGui::SetNextWindowPos({ panel_width, 0 });
        ImGui::SetNextWindowSize({ w - panel_width, panel_y });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, from_rgba(0x2d, 0x37, 0x40, 0xff));
        ImGui::Begin("Toolbar Panel", nullptr, flags);

        ImGui::PushStyleColor(ImGuiCol_Border, black);
        ImGui::SetCursorPosX(w - panel_width - panel_y * 3);
        ImGui::PushStyleColor(ImGuiCol_Text, is_3d_view ? grey : white);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, is_3d_view ? grey : white);
        if (ImGui::Button("2D", { panel_y,panel_y })) is_3d_view = false;
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
        ImGui::SetCursorPosX(w - panel_width - panel_y * 2);
        ImGui::PushStyleColor(ImGuiCol_Text, !is_3d_view ? grey : white);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, !is_3d_view ? grey : white);
        if (ImGui::Button("3D", { panel_y,panel_y })) is_3d_view = true;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::SetCursorPosX(w - panel_width - panel_y);

        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, light_blue);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if (ImGui::Button(u8"\uf013\uf0d7", { panel_y,panel_y }))
            ImGui::OpenPopup("global_menu");

        ImGui::PushFont(font_14);
        if (ImGui::BeginPopup("global_menu"))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, dark_grey);
            if (ImGui::Selectable("About RealSense Viewer"))
            {
            }

            ImGui::PopStyleColor();
            ImGui::EndPopup();
        }
        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        viewer_model.show_event_log(font_14, panel_width,
            h - (viewer_model.is_output_collapsed ? default_log_h : 20),
            w - panel_width, default_log_h);

        // Set window position and size
        ImGui::SetNextWindowPos({ 0, panel_y });
        ImGui::SetNextWindowSize({ panel_width, h - panel_y });
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, from_rgba(0x1b, 0x21, 0x25, 0xff));

        // *********************
        // Creating window menus
        // *********************
        ImGui::Begin("Control Panel", nullptr, flags | ImGuiWindowFlags_AlwaysVerticalScrollbar);

        if (list.size() > 0)
        {
            for (auto&& dev_model : device_models)
            {
                ImGui::PushFont(font_14);
                auto pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(pos, { pos.x + panel_width, pos.y + panel_y }, ImColor(sensor_header_light_blue));
                ImGui::GetWindowDrawList()->AddLine({ pos.x,pos.y }, { pos.x + panel_width,pos.y }, ImColor(black));

                ImGui::PushStyleColor(ImGuiCol_Button, sensor_header_light_blue);
                ImGui::Columns(2, "DeviceInfo", false);
                ImGui::SetCursorPos({ 5, 14 });
                label = to_string() << u8"\uf03d  " << dev_model.dev.get_info(RS2_CAMERA_INFO_NAME);
                ImGui::Text(label.c_str());
                ImGui::NextColumn();
                ImGui::SetCursorPos({ ImGui::GetCursorPosX(), 14 });
                label = to_string() << "S/N: " << (dev_model.dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER) ? dev_model.dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "Unknown");
                ImGui::Text(label.c_str());

                ImGui::Columns(1);
                ImGui::SetCursorPos({ panel_width - 45, 11 });

                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);
                ImGui::PushStyleColor(ImGuiCol_PopupBg, almost_white_bg);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, from_rgba(0, 0xae, 0xff, 255));
                ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, white);

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

                label = to_string() << "device_menu" << dev_model.id;

                if (ImGui::Button(u8"\uf013\uf0d7", { 25,25 }))
                    ImGui::OpenPopup(label.c_str());

                ImGui::PushFont(font_14);

                if (ImGui::BeginPopup(label.c_str()))
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, dark_grey);
                    if (ImGui::Selectable("Show Device Details..."))
                    {

                    }

                    if (ImGui::Selectable("Collapse All"))
                    {

                    }

                    if (auto adv = dev_model.dev.as<advanced_mode>())
                    {
                        ImGui::Separator();

                        if (ImGui::Selectable("Load Settings", false, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            auto ret = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "JSON\0*.json\0", NULL, NULL);
                            if (ret)
                            {
                                std::ifstream t(ret);
                                std::string str((std::istreambuf_iterator<char>(t)),
                                                std::istreambuf_iterator<char>());

                                adv.load_json(str);
                                get_curr_advanced_controls = true;
                            }
                        }

                        if (ImGui::Selectable("Save Settings", false, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            auto ret = noc_file_dialog_open(NOC_FILE_DIALOG_SAVE, "JSON\0*.json\0", NULL, NULL);

                            if (ret)
                            {
                                std::string filename = ret;
                                if (!ends_with(to_lower(filename), ".json")) filename += ".json";

                                std::ofstream out(filename);
                                out << adv.serialize_json();
                                out.close();
                            }
                        }
                    }

                    ImGui::Separator();

                    if (ImGui::Selectable("Hardware Reset"))
                    {
                        try
                        {
                            restarting_device_info = get_device_info(dev_model.dev, false);
                            dev_model.dev.hardware_reset();
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

                    ImGui::Separator();

                    if (ImGui::Selectable("Remove Source"))
                    {
                        for (auto&& sub : dev_model.subdevices)
                        {
                            if (sub->streaming)
                                sub->stop();
                        }
                        device_to_remove = &dev_model;
                    }

                    ImGui::PopStyleColor();
                    ImGui::EndPopup();
                }
                ImGui::PopFont();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(4);

                ImGui::SetCursorPos({ 0, panel_y });

                ImGui::PopStyleColor();
                ImGui::PopFont();


                auto windows_width = ImGui::GetContentRegionMax().x;
                auto sensor_top_y = ImGui::GetCursorPosY();
                ImGui::SetContentRegionWidth(windows_width - 36);

                std::map<subdevice_model*, float> model_to_y;
                std::map<subdevice_model*, float> model_to_abs_y;

                static bool is_recording = false;
                static char input_file_name[256] = "recorded_streams.bag";

                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, from_rgba(0x1b, 0x21, 0x25, 0xff));
                ImGui::PushStyleColor(ImGuiCol_Text, from_rgba(0xc3, 0xd5, 0xe5, 0xff));
                ImGui::PushFont(font_14);

                // Draw menu foreach subdevice with its properties
                for (auto&& sub : dev_model.subdevices)
                {
                    const ImVec2 pos = ImGui::GetCursorPos();
                    const ImVec2 abs_pos = ImGui::GetCursorScreenPos();
                    model_to_y[sub.get()] = pos.y;
                    model_to_abs_y[sub.get()] = abs_pos.y;
                    ImGui::GetWindowDrawList()->AddLine({ abs_pos.x, abs_pos.y - 2 },
                    { abs_pos.x + panel_width, abs_pos.y - 2 },
                        ImColor(black), 2.f);

                    label = to_string() << sub->s.get_info(RS2_CAMERA_INFO_NAME);
                    ImGui::PushStyleColor(ImGuiCol_Header, sensor_header_light_blue);

                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 10 });
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 0, 0 });
                    if (ImGui::TreeNode(label.c_str()))
                    {
                        anything_started = true;
                        ImGui::PopStyleVar();
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2, 2 });

                        sub->draw_stream_selection();


                        static const std::vector<rs2_option> drawing_order{
                            RS2_OPTION_ADVANCED_MODE_PRESET,
                            RS2_OPTION_EMITTER_ENABLED,
                            RS2_OPTION_ENABLE_AUTO_EXPOSURE };

                        for (auto& opt : drawing_order)
                        {
                            if (sub->draw_option(opt, update_read_only_options, error_message, viewer_model.not_model))
                            {
                                get_curr_advanced_controls = true;
                            }
                        }

                        label = to_string() << "Controls ##" << sub->s.get_info(RS2_CAMERA_INFO_NAME);
                        if (ImGui::TreeNode(label.c_str()))
                        {
                            for (auto i = 0; i < RS2_OPTION_COUNT; i++)
                            {
                                auto opt = static_cast<rs2_option>(i);
                                if (std::find(drawing_order.begin(), drawing_order.end(), opt) == drawing_order.end())
                                {
                                    if (sub->draw_option(opt, update_read_only_options, error_message, viewer_model.not_model))
                                    {
                                        get_curr_advanced_controls = true;
                                    }
                                }
                            }

                            if (auto ds = sub->s.as<depth_sensor>())
                                viewer_model.draw_histogram_options(ds.get_depth_scale(), *sub);

                            ImGui::TreePop();
                        }

                        if (dev_model.dev.is<advanced_mode>() && sub->s.is<depth_sensor>())
                            draw_advanced_mode_tab(dev_model.dev, amc, restarting_device_info, get_curr_advanced_controls);

                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

                        ImGui::TreePop();
                    }

                    ImGui::PopStyleVar();
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                }

                for (auto&& device_model : device_models)
                    for (auto&& sub : device_model.subdevices)
                    {
                        sub->update(error_message, viewer_model.not_model);
                    }

                ImGui::PopStyleColor(2);
                ImGui::PopFont();

                ImGui::SetContentRegionWidth(windows_width);

                for (auto&& sub : dev_model.subdevices)
                {
                    try
                    {
                        static float t = 0.f;
                        t += 0.03f; // TODO: change to something more elegant

                        ImGui::SetCursorPos({ windows_width - 32, model_to_y[sub.get()] + 3 });
                        ImGui::PushFont(font_14);
                        if (sub.get() == dev_model.subdevices.begin()->get() && !anything_started)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Button, from_rgba(0x1b + abs(sin(t)) * 40, 0x21 + abs(sin(t)) * 20, 0x25 + abs(sin(t)) * 30, 0xff));
                        }
                        else
                        {
                            ImGui::PushStyleColor(ImGuiCol_Button, sensor_bg);
                        }
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, sensor_bg);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, sensor_bg);

                        if (!sub->streaming)
                        {
                            label = to_string() << u8"  \uf204\noff   ##" << sub->s.get_info(RS2_CAMERA_INFO_NAME);

                            ImGui::PushStyleColor(ImGuiCol_Text, redish);
                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, redish + 0.1f);

                            if (sub->is_selected_combination_supported())
                            {
                                if (ImGui::Button(label.c_str(), { 30,30 }))
                                {
                                    anything_started = true;
                                    //if (is_recording)
                                    //{
                                    //    model.start_recording(dev, input_file_name, error_message);
                                    //}
                                    auto profiles = sub->get_selected_profiles();
                                    sub->play(profiles);

                                    for (auto&& profile : profiles)
                                    {
                                        viewer_model.streams[profile.unique_id()].dev = sub;
                                    }
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Start streaming data from this sensor");
                                    anything_started = true;
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled(u8"  \uf204\noff   ");
                            }
                        }
                        else
                        {
                            label = to_string() << u8"  \uf205\n    on##" << sub->s.get_info(RS2_CAMERA_INFO_NAME);
                            ImGui::PushStyleColor(ImGuiCol_Text, light_blue);
                            ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, light_blue + 0.1f);

                            if (ImGui::Button(label.c_str(), { 30,30 }))
                            {
                                sub->stop();
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
            }

            if (device_to_remove)
            {
                device_models.erase(std::find_if(begin(device_models), end(device_models),
                    [&](const device_model& other) { return get_device_name(other.dev) == get_device_name(device_to_remove->dev); }));
                device_to_remove = nullptr;
            }
        }

        {
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            auto h = ImGui::GetWindowHeight();
            if (h > pos.y)
            {
                ImGui::GetWindowDrawList()->AddLine({ pos.x,pos.y }, { pos.x + panel_width,pos.y }, ImColor(from_rgba(0, 0, 0, 0xff)));
                ImRect bb(pos, ImVec2(pos.x + ImGui::GetContentRegionAvail().x, pos.y + ImGui::GetContentRegionAvail().y));
                ImGui::GetWindowDrawList()->AddRectFilled(bb.GetTL(), bb.GetBR(), ImColor(dark_window_background));
            }
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

        // Rendering
        glViewport(0, 0,
            static_cast<int>(ImGui::GetIO().DisplaySize.x),
            static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        auto output_height = (viewer_model.is_output_collapsed ? default_log_h : 20);

        if (!is_3d_view)
        {
            glfwGetWindowSize(window, &w, &h);
            glLoadIdentity();
            glOrtho(0, w, h, 0, -1, +1);

            auto layout = viewer_model.calc_layout(panel_width, panel_y, w - panel_width, (float)h - panel_y - output_height);

            if (layout.size() == 0 && list.size() > 0)
            {
                viewer_model.show_no_stream_overlay(font_18, panel_width, panel_y, w, (float)h - output_height);
            }

            for (auto &&kvp : layout)
            {
                auto&& view_rect = kvp.second;
                auto stream = kvp.first;
                auto&& stream_mv = viewer_model.streams[stream];
                auto&& stream_size = stream_mv.size;
                auto stream_rect = view_rect.adjust_ratio(stream_size);

                stream_mv.show_frame(stream_rect, mouse, error_message);

                if (stream_mv.dev->is_paused())
                    viewer_model.show_paused_icon(font_18, stream_rect.x + 5, stream_rect.y + 5, stream_mv.profile.unique_id());

                stream_mv.show_stream_header(font_14, stream_rect, viewer_model);
                stream_mv.show_stream_footer(stream_rect, mouse);
            }

            // Metadata overlay windows shall be drawn after textures to preserve z-buffer functionality
            for (auto &&kvp : layout)
            {
                if (viewer_model.streams[kvp.first].metadata_displayed)
                    viewer_model.streams[kvp.first].show_metadata(mouse);
            }
        }
        else
        {
            rect viewer_rect = { panel_width, panel_y, w - panel_width, h - panel_y - output_height };
            show_3dviewer_header(font_14, viewer_rect, viewer_model);

            static auto last_frame_number = 0;
            for (auto&& s : viewer_model.streams)
            {
                if (s.second.profile.stream_type() == RS2_STREAM_DEPTH && s.second.texture->last)
                {
                    auto frame_number = s.second.texture->last.get_frame_number();

                    if (last_frame_number == frame_number) break;
                    last_frame_number = frame_number;

                    for (auto&& s : viewer_model.streams)
                    {
                        if (s.second.profile.stream_type() != RS2_STREAM_DEPTH && s.second.texture->last)
                        {
                            rendered_tex_id = s.second.texture->get_gl_handle();
                            pc.map_to(s.second.texture->last);
                            break;
                        }
                    }

                    if (s.second.dev && !s.second.dev->is_paused())
                        depth_frames_to_render.enqueue(s.second.texture->last);
                    break;
                }
            }

            if (paused)
                viewer_model.show_paused_icon(font_18, panel_width + 15, panel_y + 15 + 32, 0);

            frame f;
            if (resulting_3d_models.poll_for_frame(&f))
            {
                viewer_model.model_3d = f;
            }

            glfwGetFramebufferSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0, 0, 0, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto sec_since_update = std::chrono::duration<double, std::milli>(now - view_clock).count();
            view_clock = now;

            if (viewer_rect.contains({ mouse.cursor.x, mouse.cursor.y }))
                arcball_camera_update(
                (float*)&pos, (float*)&target, (float*)&up, view,
                    sec_since_update,
                    0.2f, // zoom per tick
                    1.5f, // pan speed
                    3.0f, // rotation multiplier
                    w, h, // screen (window) size
                    oldcursor.x, mouse.cursor.x,
                    oldcursor.y, mouse.cursor.y,
                    ImGui::GetIO().MouseDown[2],
                    ImGui::GetIO().MouseDown[0],
                    mouse.mouse_wheel,
                    0);

            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glLoadIdentity();
            gluPerspective(60, (float)w / h, 0.001f, 100.0f);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glLoadMatrixf(view);

            glDisable(GL_TEXTURE_2D);

            glEnable(GL_DEPTH_TEST);

            glLineWidth(1);
            glBegin(GL_LINES);
            glColor4f(0.4f, 0.4f, 0.4f, 1.f);

            glTranslatef(0, 0, -1);

            for (int i = 0; i <= 6; i++)
            {
                glVertex3f(i - 3, 1, 0);
                glVertex3f(i - 3, 1, 6);
                glVertex3f(-3, 1, i);
                glVertex3f(3, 1, i);
            }
            glEnd();

            texture_buffer::draw_axis(0.1, 1);

            glColor4f(1.f, 1.f, 1.f, 1.f);
            glEnable(GL_TEXTURE_2D);

            if (auto points = viewer_model.model_3d)
            {
                glPointSize((float)w / points.get_profile().as<video_stream_profile>().width());

                glBindTexture(GL_TEXTURE_2D, rendered_tex_id);

                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_border_mode);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_border_mode);

                glBegin(GL_POINTS);

                auto vertices = points.get_vertices();
                auto tex_coords = points.get_texture_coordinates();

                for (int i = 0; i < points.size(); i++)
                {
                    if (vertices[i].z)
                    {
                        glVertex3fv(vertices[i]);
                        glTexCoord2fv(tex_coords[i]);
                    }

                }

                glEnd();
            }

            glDisable(GL_DEPTH_TEST);

            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glPopAttrib();


            if (ImGui::IsKeyPressed('R') || ImGui::IsKeyPressed('r'))
            {
                reset_camera(pos, target, up);
            }

        }

        if (ImGui::IsKeyPressed(' '))
        {
            if (paused)
            {
                for (auto&& s : viewer_model.streams)
                {
                    if (s.second.dev) s.second.dev->resume();
                }
            }
            else
            {
                for (auto&& s : viewer_model.streams)
                {
                    if (s.second.dev) s.second.dev->pause();
                }
            }
            paused = !paused;
        }

        viewer_model.not_model.draw(font_14, w, h);

        viewer_model.popup_if_error(font_14, error_message);

        ImGui::Render();
        glfwSwapBuffers(window);
        mouse.mouse_wheel = 0;
        oldcursor = mouse.cursor;
    }

    keep_calculating_pointcloud = false;
    pointcloud_thread.join();

    // Stop all subdevices
    for (auto&& device_model : device_models)
        for (auto&& sub : device_model.subdevices)
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

#ifdef WIN32
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
) {
    main(0, nullptr);
}
#endif