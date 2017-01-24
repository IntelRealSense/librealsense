#include <librealsense/rs.hpp>
#include "example.hpp"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"

#include <cstdarg>
#include <thread>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <sstream>
#include <mutex>

#pragma comment(lib, "opengl32.lib")

class option_model
{
public:
    rs_option opt;
    rs::option_range range;
    rs::device endpoint;
    bool* invalidate_flag;
    bool supported = false;
    float value = 0.0f;
    std::string label = "";
    std::string id = "";

    void draw(std::string& error_message)
    {
        if (supported)
        {
            if (is_checkbox())
            {
                auto bool_value = value > 0.0f;
                if (ImGui::Checkbox(label.c_str(), &bool_value))
                {
                    value = bool_value ? 1.0f : 0.0f;
                    try
                    {
                        endpoint.set_option(opt, value);
                        *invalidate_flag = true;
                    }
                    catch (const rs::error& e)
                    {
                        error_message = error_to_string(e);
                    }
                }
            }
            else
            {
                std::string txt = to_string() << label << ":";
                ImGui::Text(txt.c_str());
                ImGui::PushItemWidth(-1);

                try
                {
                    if (is_enum())
                    {
                        std::vector<const char*> labels;
                        auto selected = 0, counter = 0;
                        for (auto i = range.min; i <= range.max; i += range.step, counter++)
                        {
                            if (abs(i - value) < 0.001f) selected = counter;
                            labels.push_back(endpoint.get_option_value_description(opt, i));
                        }
                        if (ImGui::Combo(id.c_str(), &selected, labels.data(),
                            static_cast<int>(labels.size())))
                        {
                            value = range.min + range.step * selected;
                            endpoint.set_option(opt, value);
                            *invalidate_flag = true;
                        }
                    }
                    else if (is_all_integers())
                    {
                        auto int_value = static_cast<int>(value);
                        if (ImGui::SliderInt(id.c_str(), &int_value,
                            static_cast<int>(range.min),
                            static_cast<int>(range.max)))
                        {
                            // TODO: Round to step?
                            value = static_cast<float>(int_value);
                            endpoint.set_option(opt, value);
                            *invalidate_flag = true;
                        }
                    }
                    else
                    {
                        if (ImGui::SliderFloat(id.c_str(), &value,
                            range.min, range.max))
                        {
                            endpoint.set_option(opt, value);
                        }
                    }
                }
                catch (const rs::error& e)
                {
                    error_message = error_to_string(e);
                }
                ImGui::PopItemWidth();
            }

            auto desc = endpoint.get_option_description(opt);
            if (ImGui::IsItemHovered() && desc)
            {
                ImGui::SetTooltip(desc);
            }
        }
    }

    void update(std::string& error_message)
    {
        try
        {
            if (endpoint.supports(opt))
                value = endpoint.get_option(opt);
        }
        catch (const rs::error& e)
        {
            error_message = error_to_string(e);
        }
    }
private:
    bool is_all_integers() const
    {
        return is_integer(range.min) && is_integer(range.max) &&
            is_integer(range.def) && is_integer(range.step);
    }

    bool is_enum() const
    {
        for (auto i = range.min; i <= range.max; i += range.step)
        {
            if (endpoint.get_option_value_description(opt, i) == nullptr)
                return false;
        }
        return true;
    }

    bool is_checkbox() const
    {
        return range.max == 1.0f &&
            range.min == 0.0f &&
            range.step == 1.0f;
    }
};

template<class T>
void push_back_if_not_exists(std::vector<T>& vec, T value)
{
    auto it = std::find(vec.begin(), vec.end(), value);
    if (it == vec.end()) vec.push_back(value);
}

std::vector<const char*> get_string_pointers(const std::vector<std::string>& vec)
{
    std::vector<const char*> res;
    for (auto&& s : vec) res.push_back(s.c_str());
    return res;
}

class subdevice_model
{
public:
    subdevice_model(rs::device dev, std::string& error_message)
        : dev(dev), streaming(false), queues(RS_STREAM_COUNT)
    {
        for (auto& elem : queues)
        {
            elem = std::unique_ptr<rs::frame_queue>(new rs::frame_queue(5));
        }

        for (auto i = 0; i < RS_OPTION_COUNT; i++)
        {
            option_model metadata;
            auto opt = static_cast<rs_option>(i);

            std::stringstream ss;
            ss << dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME)
                << "/" << dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME)
                << "/" << rs_option_to_string(opt);
            metadata.id = ss.str();
            metadata.opt = opt;
            metadata.endpoint = dev;
            metadata.label = rs_option_to_string(opt);
            metadata.invalidate_flag = &options_invalidated;

            metadata.supported = dev.supports(opt);
            if (metadata.supported)
            {
                try
                {
                    metadata.range = dev.get_option_range(opt);
                    metadata.value = dev.get_option(opt);
                }
                catch (const rs::error& e)
                {
                    metadata.range = { 0, 1, 0, 0 };
                    metadata.value = 0;
                    error_message = error_to_string(e);
                }
            }
            options_metadata[opt] = metadata;
        }

        try
        {
            auto uvc_profiles = dev.get_stream_modes();
            std::reverse(std::begin(uvc_profiles), std::end(uvc_profiles));
            for (auto&& profile : uvc_profiles)
            {
                std::stringstream res;
                res << profile.width << " x " << profile.height;
                push_back_if_not_exists(res_values, std::pair<int, int>(profile.width, profile.height));
                push_back_if_not_exists(resolutions, res.str());
                std::stringstream fps;
                fps << profile.fps;
                push_back_if_not_exists(fps_values, profile.fps);
                push_back_if_not_exists(fpses, fps.str());
                std::string format = rs_format_to_string(profile.format);

                push_back_if_not_exists(formats[profile.stream], format);
                push_back_if_not_exists(format_values[profile.stream], profile.format);

                auto any_stream_enabled = false;
                for (auto it : stream_enabled)
                {
                    if (it.second)
                    {
                        any_stream_enabled = true;
                        break;
                    }
                }
                if (!any_stream_enabled)
                {
                    stream_enabled[profile.stream] = true;
                }

                profiles.push_back(profile);
            }

            // set default selections
            int selection_index;

            get_default_selection_index(res_values, std::pair<int,int>(640,480), &selection_index);
            selected_res_id = selection_index;

            get_default_selection_index(fps_values, 30, &selection_index);
            selected_fps_id = selection_index;

            for (auto format_array : format_values)
            {
                for (auto format : { rs_format::RS_FORMAT_RGB8,
                                     rs_format::RS_FORMAT_Z16,
                                     rs_format::RS_FORMAT_Y8,
                                     rs_format::RS_FORMAT_MOTION_XYZ32F } )
                {
                    if (get_default_selection_index(format_array.second, format, &selection_index))
                    {
                        selected_format_id[format_array.first] = selection_index;
                        break;
                    }
                }
            }
        }
        catch (const rs::error& e)
        {
            error_message = error_to_string(e);
        }
    }

    bool is_selected_combination_supported()
    {
        std::vector<rs::stream_profile> results;

        for (auto i = 0; i < RS_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs_stream>(i);
            if (stream_enabled[stream])
            {
                auto width = res_values[selected_res_id].first;
                auto height = res_values[selected_res_id].second;
                auto fps = fps_values[selected_fps_id];
                auto format = format_values[stream][selected_format_id[stream]];

                for (auto&& p : profiles)
                {
                    if (p.width == width && p.height == height && p.fps == fps && p.format == format)
                        results.push_back(p);
                }
            }
        }
        return results.size() > 0;
    }

    std::vector<rs::stream_profile> get_selected_profiles()
    {
        std::vector<rs::stream_profile> results;

        std::stringstream error_message;
        error_message << "The profile ";

        for (auto i = 0; i < RS_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs_stream>(i);
            if (stream_enabled[stream])
            {
                auto width = res_values[selected_res_id].first;
                auto height = res_values[selected_res_id].second;
                auto fps = fps_values[selected_fps_id];
                auto format = format_values[stream][selected_format_id[stream]];

                error_message << "\n{" << rs_stream_to_string(stream) << ","
                    << width << "x" << height << " at " << fps << "Hz, "
                    << rs_format_to_string(format) << "} ";

                for (auto&& p : profiles)
                {
                    if (p.width == width &&
                        p.height == height &&
                        p.fps == fps &&
                        p.format == format &&
                        p.stream == stream)
                        results.push_back(p);
                }
            }
        }
        if (results.size() == 0)
        {
            error_message << " is unsupported!";
            throw std::runtime_error(error_message.str());
        }
        return results;
    }

    void stop()
    {
        streaming = false;

        for (auto& elem : queues)
            elem->flush();

        dev.stop();
        dev.close();
    }

    void play(const std::vector<rs::stream_profile>& profiles)
    {
        dev.open(profiles);
        try {
            dev.start([&](rs::frame f){
                auto stream_type = f.get_stream_type();
                queues[(int)stream_type]->enqueue(std::move(f));
            });
        }
        catch (...)
        {
            dev.close();
            throw;
        }

        streaming = true;
    }

    void update(std::string& error_message)
    {
        if (options_invalidated)
        {
            next_option = 0;
            options_invalidated = false;
        }
        if (next_option < RS_OPTION_COUNT)
        {
            options_metadata[static_cast<rs_option>(next_option)].update(error_message);
            next_option++;
        }
    }

    template<typename T>
    bool get_default_selection_index(const std::vector<T>& values, const T & def, int* index)
    {
        auto max_default = values.begin();
        for (auto it = values.begin(); it != values.end(); it++)
        {

            if (*it == def)
            {
                *index = (int)(it - values.begin());
                return true;
            }
            if (*max_default < *it)
            {
                max_default = it;
            }
        }
        *index = (int)(max_default - values.begin());
        return false;
    }

    rs::device dev;

    std::map<rs_option, option_model> options_metadata;
    std::vector<std::string> resolutions;
    std::vector<std::string> fpses;
    std::map<rs_stream, std::vector<std::string>> formats;
    std::map<rs_stream, bool> stream_enabled;

    int selected_res_id = 0;
    int selected_fps_id = 0;
    std::map<rs_stream, int> selected_format_id;

    std::vector<std::pair<int, int>> res_values;
    std::vector<int> fps_values;
    std::map<rs_stream, std::vector<rs_format>> format_values;

    std::vector<rs::stream_profile> profiles;

    std::vector<std::unique_ptr<rs::frame_queue>> queues;
    bool options_invalidated = false;
    int next_option = RS_OPTION_COUNT;
    bool streaming;
};


class fps_calc
{
public:
    fps_calc()
        : _counter(0),
          _delta(0),
          _last_timestamp(0),
          _num_of_frames(0)
    {}

      fps_calc(const fps_calc& other) {
        std::lock_guard<std::mutex> lock(other._mtx);
        _counter = other._counter;
        _delta = other._delta;
        _num_of_frames = other._num_of_frames;
        _last_timestamp = other._last_timestamp;
      }

    void add_timestamp(double timestamp, unsigned long long frame_counter)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (++_counter >= _skip_frames)
        {
            if (_last_timestamp != 0)
            {
                _delta = timestamp - _last_timestamp;
                _num_of_frames = frame_counter - _last_frame_counter;
            }

            _last_frame_counter = frame_counter;
            _last_timestamp = timestamp;
            _counter = 0;
        }
    }

    double get_fps()
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_delta == 0)
            return 0;

        return (static_cast<double>(_numerator) * _num_of_frames)/_delta;
    }

private:
    static const int _numerator = 1000;
    static const int _skip_frames = 5;
    unsigned long long _num_of_frames;
    int _counter;
    double _delta;
    double _last_timestamp;
    unsigned long long _last_frame_counter;
    mutable std::mutex _mtx;
};

typedef std::map<rs_stream, rect> streams_layout;

class device_model
{
public:
    explicit device_model(rs::device& dev, std::string& error_message)
    {
        for (auto&& sub : dev.get_adjacent_devices())
        {
            auto model = std::make_shared<subdevice_model>(sub, error_message);
            subdevices.push_back(model);
        }
    }


    bool is_stream_visible(rs_stream s)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - steam_last_frame[s];
        auto ms = duration_cast<milliseconds>(diff).count();
        return ms <= _frame_timeout + _min_timeout;
    }

    float get_stream_alpha(rs_stream s)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        auto diff = now - steam_last_frame[s];
        auto ms = duration_cast<milliseconds>(diff).count();
        auto t = smoothstep(static_cast<float>(ms),
            _min_timeout, _min_timeout + _frame_timeout);
        return 1.0f - t;
    }

    std::map<rs_stream, rect> calc_layout(float x0, float y0, float width, float height)
    {
        std::vector<rs_stream> active_streams;
        for (auto i = 0; i < RS_STREAM_COUNT; i++)
        {
            auto stream = static_cast<rs_stream>(i);
            if (is_stream_visible(stream))
            {
                active_streams.push_back(stream);
            }
        }

        if (fullscreen)
        {
            auto it = std::find(begin(active_streams), end(active_streams), selected_stream);
            if (it == end(active_streams)) fullscreen = false;
        }

        std::map<rs_stream, rect> results;

        if (fullscreen)
        {
            results[selected_stream] = { static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(width), static_cast<float>(height) };
        }
        else
        {
            auto factor = ceil(sqrt(active_streams.size()));
            auto complement = ceil(active_streams.size() / factor);

            auto cell_width = static_cast<float>(width / factor);
            auto cell_height = static_cast<float>(height / complement);

            auto i = 0;
            for (auto x = 0; x < factor; x++)
            {
                for (auto y = 0; y < complement; y++)
                {
                    if (i == active_streams.size()) break;

                    rect r = { x0 + x * cell_width, y0 + y * cell_height,
                        cell_width, cell_height };
                    results[active_streams[i]] = r;
                    i++;
                }
            }
        }

        return get_interpolated_layout(results);
    }

    std::vector<std::shared_ptr<subdevice_model>> subdevices;
    std::map<rs_stream, texture_buffer> stream_buffers;
    std::map<rs_stream, float2> stream_size;
    std::map<rs_stream, rs_format> stream_format;
    std::map<rs_stream, std::chrono::high_resolution_clock::time_point> steam_last_frame;
    std::map<rs_stream, double> stream_timestamp;
    std::map<rs_stream, unsigned long long> stream_frame_number;
    std::map<rs_stream, rs_timestamp_domain> stream_timestamp_domain;
    std::map<rs_stream, fps_calc> stream_fps;

    bool fullscreen = false;
    rs_stream selected_stream = RS_STREAM_ANY;

private:
    std::map<rs_stream, rect> get_interpolated_layout(const std::map<rs_stream, rect>& l)
    {
        using namespace std::chrono;
        auto now = high_resolution_clock::now();
        if (l != _layout) // detect layout change
        {
            _transition_start_time = now;
            _old_layout = _layout;
            _layout = l;
        }

        //if (_old_layout.size() == 0 && l.size() == 1) return l;

        auto diff = now - _transition_start_time;
        auto ms = duration_cast<milliseconds>(diff).count();
        auto t = smoothstep(static_cast<float>(ms), 0, 100);

        std::map<rs_stream, rect> results;
        for (auto&& kvp : l)
        {
            auto stream = kvp.first;
            if (_old_layout.find(stream) == _old_layout.end())
            {
                _old_layout[stream] = _layout[stream].center();
            }
            results[stream] = _old_layout[stream].lerp(t, _layout[stream]);
        }

        return results;
    }

    float _frame_timeout = 700.0f;
    float _min_timeout = 90.0f;

    streams_layout _layout;
    streams_layout _old_layout;
    std::chrono::high_resolution_clock::time_point _transition_start_time;
};

bool no_device_popup(GLFWwindow* window, const ImVec4& clear_color)
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        int w, h;
        glfwGetWindowSize(window, &w, &h);

        ImGui_ImplGlfw_NewFrame();

        // Rendering
        glViewport(0, 0,
            static_cast<int>(ImGui::GetIO().DisplaySize.x),
            static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // Flags for pop-up window - no window resize, move or collaps
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        // Setting pop-up window size and position
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ static_cast<float>(w), static_cast<float>(h) });
        ImGui::Begin("", nullptr, flags);

        ImGui::OpenPopup("config-ui"); // pop-up title
        if (ImGui::BeginPopupModal("config-ui", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize))
        {
            // Show Error Message
            ImGui::Text("No device detected. Is it plugged in?");
            ImGui::Separator();

            // Present options to user
            if (ImGui::Button("Retry", ImVec2(120, 0)))
            {
                return true; // Retry to find connected device
            }
            ImGui::SameLine();
            if (ImGui::Button("Exit", ImVec2(120, 0)))
            {
                return false; // No device - exit the application
            }

            ImGui::EndPopup();
        }

        ImGui::End();
        ImGui::Render();
        glfwSwapBuffers(window);
    }
    return false;
}

int main(int, char**) try
{
    // activate logging to console
    rs::log_to_console(RS_LOG_SEVERITY_INFO);

    // Init GUI
    if (!glfwInit())
        exit(1);

    // Create GUI Windows
    auto window = glfwCreateWindow(1280, 720, "librealsense - config-ui", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(10, 0, 0);

    // Create RealSense Context
    rs::context ctx;
    auto device_index = 0;
    auto list = ctx.query_devices(); // Query RealSense connected devices list

    // If no device is connected...
    while (list.size() == 0)
    {
        // if user has no connected device - exit application
        if (!no_device_popup(window, clear_color))
            return EXIT_SUCCESS;

        // else try to query again
        list = ctx.query_devices();
    }

    std::vector<std::string> device_names;
    std::string error_message = "";
    // Initialize list with each device name and serial number
    for (uint32_t i = 0; i < list.size(); i++)
    {
        try
        {
            auto l = list[i];
            auto name = l.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME);              // retrieve device name
            auto serial = l.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER);   // retrieve device serial number
            device_names.push_back(to_string() << name << " Sn#" << serial);        // push name and sn to list
        }
        catch (...)
        {
            device_names.push_back(to_string() << "Unknown Device #" << i);
        }
    }

    auto dev = list[device_index];                  // Access first device
    auto model = device_model(dev, error_message);  // Initialize device model
    std::string label;


    // Closing the window
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        ImGui_ImplGlfw_NewFrame();

        // Flags for pop-up window - no window resize, move or collaps
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        // Set window position and size
        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 300, static_cast<float>(h) });

        // *********************
        // Creating window menus
        // *********************
        ImGui::Begin("Control Panel", nullptr, flags);

        // Device Details Menu - Elaborate details on connected devices
        if (ImGui::CollapsingHeader("Device Details", nullptr, true, true))
        {
            // Draw a combo-box with the list of connected devices
            auto device_names_chars = get_string_pointers(device_names);
            ImGui::PushItemWidth(-1);
            auto new_index = device_index;
            if (ImGui::Combo("", &new_index, device_names_chars.data(),
                static_cast<int>(device_names.size())))
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
                }
                catch (const rs::error& e)
                {
                    error_message = error_to_string(e);
                }
                catch (const std::exception& e)
                {
                    error_message = e.what();
                }
            }
            ImGui::PopItemWidth();


            // Show all device details - name, module name, serial number, FW version and location
            for (auto i = 0; i < RS_CAMERA_INFO_COUNT; i++)
            {
                auto info = static_cast<rs_camera_info>(i);
                if (dev.supports(info))
                {
                    // retrieve info property
                    std::stringstream ss;
                    ss << rs_camera_info_to_string(info) << ":";
                    auto line = ss.str();
                    ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 1.0f, 0.5f });
                    ImGui::Text(line.c_str());
                    ImGui::PopStyleColor();

                    // retrieve property value
                    ImGui::SameLine();
                    auto value = dev.get_camera_info(info);
                    ImGui::Text(value);

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip(value);
                    }
                }
            }
        }

        // Streaming Menu - Allow user to play different streams
        if (ImGui::CollapsingHeader("Streaming", nullptr, true, true))
        {
            // Draw menu foreach subdevice with its properties
            for (auto&& sub : model.subdevices)
            {

                label = to_string() << sub->dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME);
                if (ImGui::CollapsingHeader(label.c_str(), nullptr, true, true))
                {
                    // Draw combo-box with all resolution options for this device
                    auto res_chars = get_string_pointers(sub->resolutions);
                    ImGui::Text("Resolution:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label = to_string() << sub->dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME)
                        << sub->dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME) << " resolution";
                    if (sub->streaming)
                        ImGui::Text(res_chars[sub->selected_res_id]);
                    else
                        ImGui::Combo(label.c_str(), &sub->selected_res_id, res_chars.data(),
                            static_cast<int>(res_chars.size()));

                    ImGui::PopItemWidth();

                    // Draw combo-box with all FPS options for this device
                    auto fps_chars = get_string_pointers(sub->fpses);
                    ImGui::Text("FPS:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label = to_string() << sub->dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME)
                        << sub->dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME) << " fps";
                    if (sub->streaming)
                        ImGui::Text(fps_chars[sub->selected_fps_id]);
                    else
                        ImGui::Combo(label.c_str(), &sub->selected_fps_id, fps_chars.data(),
                            static_cast<int>(fps_chars.size()));

                    ImGui::PopItemWidth();

                    // Check which streams are live in current device
                    auto live_streams = 0;
                    for (auto i = 0; i < RS_STREAM_COUNT; i++)
                    {
                        auto stream = static_cast<rs_stream>(i);
                        if (sub->formats[stream].size() > 0)
                            live_streams++;
                    }

                    // Draw combo-box with all format options for current device
                    for (auto i = 0; i < RS_STREAM_COUNT; i++)
                    {
                        auto stream = static_cast<rs_stream>(i);
                        if (sub->formats[stream].size() == 0)
                            continue;

                        auto formats_chars = get_string_pointers(sub->formats[stream]);
                        if (live_streams > 1)
                        {
                            label = to_string() << rs_stream_to_string(stream) << " format:";
                            if (sub->streaming)
                                ImGui::Text(label.c_str());
                            else
                                ImGui::Checkbox(label.c_str(), &sub->stream_enabled[stream]);
                        }
                        else
                        {
                            label = to_string() << "Format:";
                            ImGui::Text(label.c_str());
                        }

                        ImGui::SameLine();
                        if (sub->stream_enabled[stream])
                        {
                            ImGui::PushItemWidth(-1);
                            label = to_string() << sub->dev.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME)
                                << sub->dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME)
                                << " " << rs_stream_to_string(stream) << " format";
                            if (sub->streaming)
                                ImGui::Text(formats_chars[sub->selected_format_id[stream]]);
                            else
                                ImGui::Combo(label.c_str(), &sub->selected_format_id[stream], formats_chars.data(),
                                    static_cast<int>(formats_chars.size()));


                            ImGui::PopItemWidth();
                        }
                        else
                        {
                            ImGui::Text("N/A");
                        }
                    }

                    try
                    {
                        if (!sub->streaming)
                        {
                            label = to_string() << "Play " << sub->dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME);

                            if (sub->is_selected_combination_supported())
                            {
                                if (ImGui::Button(label.c_str()))
                                {
                                    sub->play(sub->get_selected_profiles());
                                }
                                if (ImGui::IsItemHovered())
                                {
                                    ImGui::SetTooltip("Start streaming data from selected sub-device");
                                }
                            }
                            else
                            {
                                ImGui::TextDisabled(label.c_str());
                            }
                        }
                        else
                        {
                            label = to_string() << "Stop " << sub->dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME);
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
                    catch(const rs::error& e)
                    {
                        error_message = error_to_string(e);
                    }
                    catch(const std::exception& e)
                    {
                        error_message = e.what();
                    }
                }
            }
        }

        // Control Menu - Allow user to change cameras properties
        if (ImGui::CollapsingHeader("Control", nullptr, true, true))
        {
            for (auto&& sub : model.subdevices)
            {
                label = to_string() << sub->dev.get_camera_info(RS_CAMERA_INFO_MODULE_NAME) << " options:";
                if (ImGui::CollapsingHeader(label.c_str(), nullptr, true, false))
                {
                    for (auto i = 0; i < RS_OPTION_COUNT; i++)
                    {
                        auto opt = static_cast<rs_option>(i);
                        auto&& metadata = sub->options_metadata[opt];
                        metadata.draw(error_message);
                    }
                }
            }
        }

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
                error_message.size(), { 500,100 }, ImGuiInputTextFlags_AutoSelectAll);
            ImGui::Separator();

            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                error_message = "";
                ImGui::CloseCurrentPopup();
            }

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
                    rs::frame f;
                    if (queue->poll_for_frame(&f))
                    {
                        model.stream_buffers[f.get_stream_type()].upload(f);
                        model.steam_last_frame[f.get_stream_type()] = std::chrono::high_resolution_clock::now();
                        auto is_motion = ((f.get_format() == RS_FORMAT_MOTION_RAW) || (f.get_format() == RS_FORMAT_MOTION_XYZ32F));
                        auto width = (is_motion)? 640.f : f.get_width();
                        auto height = (is_motion)? 480.f : f.get_height();
                        model.stream_size[f.get_stream_type()] = { static_cast<float>(width),
                                                                   static_cast<float>(height)};
                        model.stream_format[f.get_stream_type()] = f.get_format();
                        model.stream_frame_number[f.get_stream_type()] = f.get_frame_number();
                        model.stream_timestamp_domain[f.get_stream_type()] = f.get_frame_timestamp_domain();
                        model.stream_timestamp[f.get_stream_type()] = f.get_timestamp();
                        model.stream_fps[f.get_stream_type()].add_timestamp(f.get_timestamp(), f.get_frame_number());
                    }
                }
                catch(const rs::error& e)
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

        auto layout = model.calc_layout(300.f, 0.f, w - 300.f, (float)h);

        for (auto kvp : layout)
        {
            auto&& view_rect = kvp.second;
            auto stream = kvp.first;
            auto&& stream_size = model.stream_size[stream];
            auto stream_rect = view_rect.adjust_ratio(stream_size);

            model.stream_buffers[stream].show(stream_rect, model.get_stream_alpha(stream));

            flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
            ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y });
            ImGui::SetNextWindowSize({ stream_rect.w, stream_rect.h });
            label = to_string() << "Stream of " << rs_stream_to_string(stream);
            ImGui::Begin(label.c_str(), nullptr, flags);

            label = to_string() << rs_stream_to_string(stream) << " "
                << stream_size.x << "x" << stream_size.y << ", "
                << rs_format_to_string(model.stream_format[stream]) << ", "
                << "Frame# " << model.stream_frame_number[stream] << ", "
                << "FPS:";

            ImGui::Text(label.c_str());
            ImGui::SameLine();

            label = to_string() << std::setprecision(2) << std::fixed << model.stream_fps[stream].get_fps();
            ImGui::Text(label.c_str());
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

            label = to_string() << "Timestamp: " << std::fixed << std::setprecision(3) << model.stream_timestamp[stream]
                                << ", Domain:";
            ImGui::Text(label.c_str());

            ImGui::SameLine();
            auto domain = model.stream_timestamp_domain[stream];
            label = to_string() << rs_timestamp_domain_to_string(domain);

            if (domain == RS_TIMESTAMP_DOMAIN_SYSTEM_TIME)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.0f, 0.0f, 1.0f });
                ImGui::Text(label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hardware Timestamp unavailable! This is often an indication of inproperly applied Kernel patch.\nPlease refer to installation.md for mode information");
                }
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::Text(label.c_str());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Specifies the clock-domain for the timestamp (hardware-clock / system-time)");
                }
            }

            ImGui::End();
            ImGui::PopStyleColor();
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
catch (const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
