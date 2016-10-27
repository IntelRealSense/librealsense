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
#include <mutex>
#include <atomic>
#include <map>
#include <sstream>


#pragma comment(lib, "opengl32.lib")

bool is_integer(float f)
{
    return abs(f - floor(f)) < 0.001f;
}

struct to_string
{
    std::ostringstream ss;
    template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
    operator std::string() const { return ss.str(); }
};

std::string error_to_string(const rs::error& e)
{
    return to_string() << e.get_failed_function() << "("
        << e.get_failed_args() << "):\n" << e.what();
}

class option_model
{
public:
    rs_option opt;
    rs::option_range range;
    rs::subdevice* endpoint;
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
                        endpoint->set_option(opt, value);
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
                            labels.push_back(endpoint->get_option_value_description(opt, i));
                        }
                        if (ImGui::Combo(id.c_str(), &selected, labels.data(), 
                            static_cast<int>(labels.size())))
                        {
                            value = range.min + range.step * selected;
                            endpoint->set_option(opt, value);
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
                            endpoint->set_option(opt, value);
                            *invalidate_flag = true;
                        }
                    }
                    else
                    {
                        if (ImGui::SliderFloat(id.c_str(), &value,
                            range.min, range.max))
                        {
                            endpoint->set_option(opt, value);
                        }
                    }
                }
                catch (const rs::error& e)
                {
                    error_message = error_to_string(e);
                }
                ImGui::PopItemWidth();
            }

            auto desc = endpoint->get_option_description(opt);
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
            if (endpoint->supports(opt))
                value = endpoint->get_option(opt);
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
            if (endpoint->get_option_value_description(opt, i) == nullptr)
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
    subdevice_model(rs_subdevice subdevice, rs::subdevice* endpoint, std::string& error_message)
        : subdevice(subdevice), endpoint(endpoint), allow_new_frames(true), queues(5)
    {
        for (auto i = 0; i < RS_OPTION_COUNT; i++)
        {
            option_model metadata;
            auto opt = static_cast<rs_option>(i);

            std::stringstream ss;
            ss << rs_subdevice_to_string(subdevice) << "/" << rs_option_to_string(opt);
            metadata.id = ss.str();
            metadata.opt = opt;
            metadata.endpoint = endpoint;

            metadata.label = rs_option_to_string(opt);
            metadata.invalidate_flag = &options_invalidated;

            metadata.supported = endpoint->supports(opt);
            if (metadata.supported)
            {
                try
                {
                    metadata.range = endpoint->get_option_range(opt);
                    metadata.value = endpoint->get_option(opt);
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
            auto uvc_profiles = endpoint->get_stream_profiles();
            for (auto&& profile : uvc_profiles)
            {
                std::stringstream res;
                res << profile.width << " x " << profile.height;
                push_back_if_not_exists(res_values, { profile.width, profile.height });
                push_back_if_not_exists(resolutions, res.str());
                std::stringstream fps;
                fps << profile.fps;
                push_back_if_not_exists(fps_values, profile.fps);
                push_back_if_not_exists(fpses, fps.str());
                std::string format = rs_format_to_string(profile.format);

                push_back_if_not_exists(formats[profile.stream], format);
                push_back_if_not_exists(format_values[profile.stream], profile.format);

                bool any = false;
                for (auto&& kvp : stream_enabled)
                {
                    if (stream_enabled[profile.stream]) any = true;
                }
                if (!any) stream_enabled[profile.stream] = true;

                profiles.push_back(profile);
            }
        }
        catch (const rs::error& e)
        {
            error_message = error_to_string(e);
        }
    }

    std::vector<rs::stream_profile> get_selected_profiles()
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
        if (results.size() == 0) throw std::runtime_error("Profile not supported!");
        return results;
    }

    void stop()
    {
        allow_new_frames = false;
        queues.flush();
        if (current_stream) current_stream->stop();
        current_stream.reset();
    }

    void play(const std::vector<rs::stream_profile>& profiles)
    {
        current_stream = std::make_shared<rs::active_stream>(endpoint->open(profiles));
        current_stream->start(queues);
        allow_new_frames = true;
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

    rs_subdevice subdevice;
    rs::subdevice* endpoint;

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
    std::shared_ptr<rs::active_stream> current_stream;
    std::atomic<bool> allow_new_frames;

    std::vector<rs::stream_profile> profiles;

    rs::frame_queue queues;
    bool options_invalidated = false;
    int next_option = RS_OPTION_COUNT;
};

typedef std::map<rs_stream, rect> streams_layout;

class device_model
{
public:
    explicit device_model(rs::device& dev, std::string& error_message)
    {
        for (auto j = 0; j < RS_SUBDEVICE_COUNT; j++)
        {
            auto subdevice = static_cast<rs_subdevice>(j);
            if (!dev.supports(subdevice)) continue;
            auto&& endpoint = dev.get_subdevice(subdevice);

            auto model = std::make_shared<subdevice_model>(subdevice, &endpoint, error_message);
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

    std::map<rs_stream, rect> calc_layout(int x0, int y0, int width, int height)
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

        auto factor = ceil(sqrt(active_streams.size()));
        auto complement = ceil(active_streams.size() / factor);

        auto cell_width = static_cast<float>(width / factor);
        auto cell_height = static_cast<float>(height / complement);

        std::map<rs_stream, rect> results;
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
        return get_interpolated_layout(results);
    }

    std::vector<std::shared_ptr<subdevice_model>> subdevices;
    std::map<rs_stream, texture_buffer> stream_buffers;
    std::map<rs_stream, float2> stream_size;
    std::map<rs_stream, rs_format> stream_format;
    std::map<rs_stream, std::chrono::high_resolution_clock::time_point> steam_last_frame;

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

int main(int, char**) try
{
    rs::log_to_console(RS_LOG_SEVERITY_WARN);
    rs::log_to_file(RS_LOG_SEVERITY_DEBUG);

    if (!glfwInit())
        exit(1);

    auto window = glfwCreateWindow(1280, 720, "librealsense - config-ui", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    ImGui_ImplGlfw_Init(window, true);

    ImVec4 clear_color = ImColor(10, 0, 0);

    rs::context ctx;
    //rs::recording_context ctx("config-ui.db");
    //rs::mock_context ctx("/media/local_admin/MULTIBOOT/hq_colorexp_depth_preset.db");
    auto device_index = 0;
    auto list = ctx.query_devices();
    auto dev = list[device_index];
    std::vector<std::string> device_names;

    std::string error_message = "";

    for (auto&& l : list)
    {
        auto d = list[device_index];
        auto name = d.get_camera_info(RS_CAMERA_INFO_DEVICE_NAME);
        auto serial = d.get_camera_info(RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER);
        device_names.push_back(to_string() << name << " Sn#" << serial);
    }

    auto model = device_model(dev, error_message);
    std::string label;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        ImGui_ImplGlfw_NewFrame();

        auto flags = ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse;

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize({ 300, static_cast<float>(h) });
        ImGui::Begin("Control Panel", nullptr, flags);

        ImGui::Text("Viewer FPS: %.0f ", ImGui::GetIO().Framerate);

        if (ImGui::CollapsingHeader("Device Details", nullptr, true, true))
        {
            auto device_names_chars = get_string_pointers(device_names);
            ImGui::PushItemWidth(-1);
            if (ImGui::Combo("", &device_index, device_names_chars.data(), 
                static_cast<int>(device_names.size())))
            {
                for (auto&& sub : model.subdevices)
                {
                    sub->stop();
                }

                dev = list[device_index];
                model = device_model(dev, error_message);
            }
            ImGui::PopItemWidth();

            for (auto i = 0; i < RS_CAMERA_INFO_COUNT; i++)
            {
                auto info = static_cast<rs_camera_info>(i);
                if (dev.supports(info))
                {
                    std::stringstream ss;
                    ss << rs_camera_info_to_string(info) << ":";
                    auto line = ss.str();
                    ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 1.0f, 0.5f });
                    ImGui::Text(line.c_str());
                    ImGui::PopStyleColor();

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

        if (ImGui::CollapsingHeader("Streaming", nullptr, true, true))
        {
            for (auto&& sub : model.subdevices)
            {
                label = to_string() << rs_subdevice_to_string(sub->subdevice);
                if (ImGui::CollapsingHeader(label.c_str(), nullptr, true, true))
                {
                    auto res_chars = get_string_pointers(sub->resolutions);
                    auto fps_chars = get_string_pointers(sub->fpses);
  
                    ImGui::Text("Resolution:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label = to_string() << rs_subdevice_to_string(sub->subdevice) << " resolution";
                    if (sub->current_stream.get()) ImGui::Text(res_chars[sub->selected_res_id]);
                    else ImGui::Combo(label.c_str(), &sub->selected_res_id, res_chars.data(), 
                                      static_cast<int>(res_chars.size()));
                    ImGui::PopItemWidth();

                    ImGui::Text("FPS:");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1);
                    label = to_string() << rs_subdevice_to_string(sub->subdevice) << " fps";
                    if (sub->current_stream.get()) ImGui::Text(fps_chars[sub->selected_fps_id]);
                    else ImGui::Combo(label.c_str(), &sub->selected_fps_id, fps_chars.data(), 
                                      static_cast<int>(fps_chars.size()));
                    ImGui::PopItemWidth();

                    auto live_streams = 0;
                    for (auto i = 0; i < RS_STREAM_COUNT; i++)
                    {
                        auto stream = static_cast<rs_stream>(i);
                        if (sub->formats[stream].size() > 0) live_streams++;
                    }

                    for (auto i = 0; i < RS_STREAM_COUNT; i++)
                    {
                        auto stream = static_cast<rs_stream>(i);
                        if (sub->formats[stream].size() == 0) continue;
                        auto formats_chars = get_string_pointers(sub->formats[stream]);
                        if (live_streams > 1)
                        {
                            label = to_string() << rs_stream_to_string(stream) << " format:";
                            if (sub->current_stream.get()) ImGui::Text(label.c_str());
                            else ImGui::Checkbox(label.c_str(), &sub->stream_enabled[stream]);
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
                            label = to_string() << rs_subdevice_to_string(sub->subdevice)
                                << " " << rs_stream_to_string(stream) << " format";
                            if (sub->current_stream.get()) ImGui::Text(formats_chars[sub->selected_format_id[stream]]);
                            else ImGui::Combo(label.c_str(), &sub->selected_format_id[stream], formats_chars.data(),
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
                        if (!sub->current_stream.get())
                        {
                            label = to_string() << "Play " << rs_subdevice_to_string(sub->subdevice);
                            if (ImGui::Button(label.c_str()))
                            {
                                sub->play(sub->get_selected_profiles());
                            }
                        }
                        else
                        {
                            label = to_string() << "Stop " << rs_subdevice_to_string(sub->subdevice);
                            if (ImGui::Button(label.c_str()))
                            {
                                sub->stop();
                            }
                        }
                    }
                    catch(const rs::error& e)
                    {
                        error_message = error_to_string(e);
                    }
                }
            }
        }

        if (ImGui::CollapsingHeader("Control", nullptr, true, true))
        {
            for (auto&& sub : model.subdevices)
            {
                label = to_string() << rs_subdevice_to_string(sub->subdevice) << " options:";
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

        for (auto&& sub : model.subdevices)
        {
            rs::frame f;
            if (sub->queues.poll_for_frame(&f))
            {
                model.stream_buffers[f.get_stream_type()].upload(f);
                model.steam_last_frame[f.get_stream_type()] = std::chrono::high_resolution_clock::now();
                model.stream_size[f.get_stream_type()] = { static_cast<float>(f.get_width()),
                                                           static_cast<float>(f.get_height()) };
                model.stream_format[f.get_stream_type()] = f.get_format();
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

        auto layout = model.calc_layout(300, 0, w - 300, h);

        for (auto kvp : layout)
        {
            auto&& view_rect = kvp.second;
            auto stream = kvp.first;
            auto&& stream_size = model.stream_size[stream];
            auto stream_rect = view_rect.adjust_ratio(model.stream_size[kvp.first]);

            model.stream_buffers[stream].show(stream_rect, model.get_stream_alpha(kvp.first));

            flags = ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar;

            ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0, 0, 0, 0 });
            ImGui::SetNextWindowPos({ stream_rect.x, stream_rect.y });
            ImGui::SetNextWindowSize({ stream_rect.w, stream_rect.h });
            label = to_string() << "Stream of " << rs_stream_to_string(kvp.first);
            ImGui::Begin(label.c_str(), nullptr, flags);

            label = to_string() << rs_stream_to_string(kvp.first) << " "
                << stream_size.x << "x" << stream_size.y << ", " 
                << rs_format_to_string(model.stream_format[stream]);
            ImGui::Text(label.c_str());
            ImGui::End();
            ImGui::PopStyleColor();
        }

        ImGui::Render();

        glfwSwapBuffers(window);
    }

    for (auto&& sub : model.subdevices)
    {
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
